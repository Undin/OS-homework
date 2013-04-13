#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

int buffer_size = 4096;

char delimiter = 'a';
int eof = 0;
int read_symbol = 0;

int next(char *buffer, int begin, int end)
{
    int i;
    for (i = begin; i < end; i++)
    {
        if (buffer[i] == delimiter)
        {
            return i;
        }
    }
    return -1;
}

void print(char *buffer, int len)
{
    int written = 0;
    while (len - written > 0)
    {
        written += write(1, buffer + written, len - written);
    }
}

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "nzb:")) != -1)
    {
        switch(opt)
        {
            case 'n':
                if (delimiter != 'a')
                {
                    exit(2);
                }
                delimiter = '\n';
                break;
            case 'z':
                if (delimiter != 'a')
                {
                    exit(2);
                }
                delimiter = '\0';
                break;
            case 'b':
                buffer_size = atoi(optarg);
                break;
            default:
                exit(2);
        }
    }

    if (delimiter == 'a')
    {
        delimiter = '\n';
    }

    char *buffer = malloc(sizeof(char) * buffer_size);
    int devnull = open("/dev/null", O_WRONLY);

    while (!eof)
    {
        int position = 0;
        int pos;
        while ((pos = next(buffer, position, read_symbol)) == -1)
        {
            position = read_symbol;
            if (read_symbol == buffer_size)
            {
                exit(1);
            }
            int res = read(0, buffer + position, buffer_size - position);
            if (res == 0)
            {   
                eof = 1;
                if (read_symbol == 0)
                {
                    pos = -1;
                    break;
                }
                buffer[read_symbol++] = delimiter;
            }
            read_symbol += res;
        }
        if (pos != -1)
        {
            char *str = malloc(sizeof(char) * (pos + 1));
            memcpy(str, buffer, pos + 1);
            str[pos] = '\0';
            char **arr = malloc(sizeof(char *) * (argc - optind + 2));
            int i;
            for (i = 0; i < argc - optind + 2; i++)
            {
                arr[i] = argv[i + optind];
            }
            arr[argc - optind] = str;
            arr[argc - optind + 1] = NULL;
            int pid;
            if ((pid = fork()) != 0)
            {
                int stat;
                waitpid(pid, &stat, 0);
                if (WIFEXITED(stat) && WEXITSTATUS(stat) == 0)
                {
                    str[pos] = '\n';
                    print(str, pos + 1);
                }
            }
            else
            {
                int i;
                dup2(devnull, 1);
                execvp(arr[0], arr);
                exit(1);
            }
            free(str);
            free(arr);
            memmove(buffer, buffer + pos + 1, sizeof(char) * (buffer_size - pos - 1));
            read_symbol -= pos + 1;
        }
    }
    close(devnull);
    return 0;
}
