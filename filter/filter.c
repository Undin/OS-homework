#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

int next(char *buffer, int begin, int end, char delimiter)
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

void print(int fd, char *buffer, int len)
{
    int written = 0;
    while (len - written > 0)
    {
        written += write(fd, buffer + written, len - written);
    }
}

void *safe_malloc(size_t size)
{
    void *tmp = malloc(size);
    if (tmp == NULL)
    {
        char error[] = "memory allocation failed\n";
        print(2, error, strlen(error));
        exit(1);
    }
    return tmp;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        exit(1);
    }
    int opt;
    int used_delimiter = 0;
    char delimiter;
    int buffer_size = 4096;

    while ((opt = getopt(argc, argv, "nzb:")) != -1)
    {
        switch(opt)
        {
            case 'n':
                if (used_delimiter != 0)
                {
                    exit(2);
                }
                delimiter = '\n';
                used_delimiter = 1;
                break;
            case 'z':
                if (used_delimiter != 0)
                {
                    exit(2);
                }
                delimiter = '\0';
                used_delimiter = 1;
                break;
            case 'b':
                buffer_size = atoi(optarg);
                break;
            default:
                exit(2);
        }
    }

    if (used_delimiter == 0)
    {
        delimiter = '\n';
        used_delimiter = 1;
    }

    char *buffer = safe_malloc(buffer_size);
    int devnull = open("/dev/null", O_WRONLY);
    int eof_flag = 0;
    int read_symbol = 0;

    while (!eof_flag)
    {
        int position = 0;
        int pos;
        while ((pos = next(buffer, position, read_symbol, delimiter)) == -1)
        {
            position = read_symbol;
            if (read_symbol == buffer_size)
            {
                exit(1);
            }
            int res = read(0, buffer + position, buffer_size - position);
            if (res == 0)
            {   
                eof_flag = 1;
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
            char *str = safe_malloc(pos + 1);
            memcpy(str, buffer, pos + 1);
            str[pos] = '\0';
            char **arr = safe_malloc(sizeof(char *) * (argc - optind + 2));
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
                    print(1, str, pos + 1);
                }
            }
            else
            {
                dup2(devnull, 1);
                execvp(arr[0], arr);
                exit(1);
            }
            free(str);
            free(arr);
            memmove(buffer, buffer + pos + 1, buffer_size - pos - 1);
            read_symbol -= pos + 1;
        }
    }
    close(devnull);
    free(buffer);
    return 0;
}
