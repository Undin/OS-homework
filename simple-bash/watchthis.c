#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

const int buffer_size = 10;
char prev_path[] = "/tmp/.watchthis/previous";
char cur_path[] = "/tmp/.watchthis/current";
char dir_path[] = "/tmp/.watchthis";
char exec_path[] = "/bin/";
char diff_command[] = "diff -u \"/tmp/.watchthis/previous\" \"/tmp/.watchthis/current\"";
char buffer[10];

void print(int fd, int k)
{
    int written = 0;
    while (k - written > 0)
    {
        int res = write(fd, buffer + written, k - written);
        written += res;
    }
}

void rewrite(int in, int out)
{
    int res = 1;
    int n;
    while (res)
    {
        res = read(in, buffer, buffer_size);
        if (res != 0)
        {
            print(out, res);
        }
    }
}

size_t mstrlen(const char *str)
{
    size_t i = 0;
    while (str[i] != '\0')
    {
        i++;
    }
    return i;
}

char* mstrcat(char *dest, const char *src)
{
    size_t dest_len = mstrlen(dest);
    size_t i;
    for (i = 0; src[i] != '\0'; i++)
    {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    return dest;
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        int len = 0;
        int i = 0;
        for (i = 2; i < argc; i++)
        {
            len += mstrlen(argv[i]);
        }
        len += argc - 1;
        char *command = malloc(len);
        command[0] = 0;
        for (i = 2; i < argc; i++)
        {
            mstrcat(command, argv[i]);
            mstrcat(command, " ");
        }
        
        int interval = atoi(argv[1]);
        mkdir(dir_path, 0777);

        int prev = open(prev_path, O_CREAT, 0666);
        int cur = open(cur_path, O_CREAT | O_TRUNC, 0666);
        close(prev);
        close(cur);
        int out = dup(1);

        while (1)
        {
            prev = open(prev_path, O_WRONLY | O_TRUNC);
            cur = open(cur_path, O_RDWR);
            rewrite(cur, prev);
            close(cur);
            cur = open(cur_path, O_WRONLY | O_TRUNC);
            dup2(cur, 1);
            system(command);
            close(cur);
            dup2(out, 1);
            cur = open(cur_path, O_RDONLY);
            rewrite(cur, 1);
            close(prev);
            close(cur);
            system(diff_command);
            sleep(interval);
        }
    }
    return 0;
}
