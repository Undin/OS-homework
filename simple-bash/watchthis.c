#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

const int buffer_size = 64;
char prev_path[] = "/tmp/.watchthis/previous";
char cur_path[] = "/tmp/.watchthis/current";
char dir_path[] = "/tmp/.watchthis";
char buffer[64];

void print(int fd, int k)
{
    int written = 0;
    while (k - written > 0)
    {
        written += write(fd, buffer + written, k - written);
    }
}

void rewrite(int in, int out)
{
    int res = 1;
    while (res)
    {
        res = read(in, buffer, buffer_size);
        if (res != 0)
        {
            print(out, res);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        char **command = malloc(sizeof(char *) * (argc - 1));
        int i;
        for (i = 0; i < argc - 2; i++)
        {
            command[i] = argv[i + 2];
        }
        command[argc - 2] = NULL;
        int interval = atoi(argv[1]);

        mkdir(dir_path, 0777);
        int prev = open(prev_path, O_CREAT, 0666);
        int cur = open(cur_path, O_CREAT | O_TRUNC, 0666);
        close(prev);
        close(cur);

        while (1)
        {
            prev = open(prev_path, O_WRONLY | O_TRUNC);
            cur = open(cur_path, O_RDWR);
            rewrite(cur, prev);
            close(cur);
            cur = open(cur_path, O_WRONLY | O_TRUNC);
            int pid;
            if ((pid = fork()) != 0)
            {
                int stat;
                waitpid(pid, &stat, 0);
                close(cur);
                cur = open(cur_path, O_RDONLY);
                rewrite(cur, 1);
                close(prev);
                close(cur);
                if ((pid = fork()) != 0)
                {
                    waitpid(pid, &stat, 0);
                    sleep(interval);
                }
                else
                {   
                    execl("/usr/bin/diff", "diff", "-u", prev_path, cur_path, NULL);
                    return 1;
                }
            }
            else
            {
                dup2(cur, 1);
                execvp(command[0], command);
                return 1;
            }
        }
        free(command);
    }
    return 0;
}
