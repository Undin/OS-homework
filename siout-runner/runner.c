#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <utility>
#include <stdlib.h>

static const int buffer_size = 4096;

std::pair<char *, int> next(int fd, char *buffer)
{
    static bool eof_flag = false;
    static int position = 0;
    static int current_position = 0; 
    while (!eof_flag)
    {
        for (int i = current_position; i < position - 1; i++)
        {
            if (buffer[i] == '\0' && buffer[i + 1] == '\0')
            {
                char *p = (char *) malloc(i);
                memcpy(p, buffer, i);
                memmove(buffer, buffer + i + 2, position - i - 2);
                position -= i + 2;
                current_position = 0;
                return std::make_pair(p, i + 1);
            }
        }
        if (position == buffer_size)
        {
            return std::pair<char *, int>(NULL, -1);
        }
        int res = read(fd, buffer + position, buffer_size - position);
        if (res == 0)
        {
            eof_flag = true;
        }
        position += res;
    }
    return std::pair<char *, int>(NULL, 0);
}

int count_of_null(char *p, int n)
{
    int count = 0;
    for (char *i = p; i < p + n; i++)
    {
        if (*i == '\0')
        {
            count++;
        }
    }
    return count;
}

char **prepare(char *p, int n, int l)
{
    int len = strlen(p) + 1;
    char **arg = (char **) malloc(sizeof(char *) * (l + 1));
    int i = 0;
    for (char *s = p + len; i < l; s += strlen(s) + 1, i++)
    {
        arg[i] = s;
    }
    arg[l] = 0;
    return arg;
}

void execution(char **arg, char *p, int n)
{
    char *s;
    int in = open(p, O_RDONLY);
    for (s = p + n - 2; *s != '\0'; s--);
    int out = open(s + 1, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    dup2(in, 0);
    dup2(out, 1);
    close(in);
    close(out);
    execvp(arg[0], arg);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        exit(1);
    }
    int in = open(argv[1], O_RDONLY);
    std::pair<char *, int> pair;
    std::vector<int> pids;
    char buffer[buffer_size];

    while (true)
    {
        pair = next(in, buffer);
        if (pair.first == NULL)
        {
            if (pair.second == -1)
            {
                exit(1);
            }
            if (pair.second == 0)
            {
                break;
            }
        }
        int count = count_of_null(pair.first, pair.second);
        if (count < 3)
        {
            exit(1);
        }
        char **arg = prepare(pair.first, pair.second, count - 2);
        int pid;
        if (!(pid = fork()))
        {
            execution(arg, pair.first, pair.second);
            exit(255);
        }
        else
        {
            pids.push_back(pid);
        }
        free(arg);
        free(pair.first);
    }
    for (size_t i = 0; i < pids.size(); i++)
    {
        int st;
        waitpid(pids[i], &st, 0);
    }
    close(in);
    return 0;
}
