#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <utility>
#include <stdlib.h>

char buffer[4096];
int buffer_size = 4096;

std::pair<char *, int> next(int fd)
{
    static bool eof = false;
    static int position = 0;
    static int current_position = 0; 
    int i;
    while (!eof)
    {
        for (i = current_position; i < position - 1; i++)
        {
            if (buffer[i] == '\0' && buffer[i + 1] == '\0')
            {
                char *p = (char *)malloc(sizeof(char) * i);
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
            eof = true;
        }
        position += res;
    }
    return std::pair<char *, int>(NULL, 0);
}

int count_of_null(char *p, int n)
{
    char *i;
    int count = 0;
    for (i = p; i < p + n; i++)
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
    char *s1, *s2;
    for (s1 = p + n - 2; *s1 != '\0'; s1--);
    int len = strlen(p) + 1;
    char **arg = (char **)malloc(sizeof(char *) * (l + 1));
    int i;
    for (s2 = p + len, i = 0; i < l; s2 += strlen(s2) + 1, i++)
    {
        arg[i] = s2;
    }
    arg[l] = 0;
    return arg;
}

void execution(char **arg, char *p, int n)
{
    char *s1, *s2;
    int in = open(p, O_RDONLY);
    for (s1 = p + n - 2; *s1 != '\0'; s1--);
    int out = open(s1 + 1, O_WRONLY | O_TRUNC | O_CREAT, 0644);
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
        return 3;
    }
    int in = open(argv[1], O_RDONLY);
    std::pair<char *, int> pair;
    std::vector<int> pids;
    while (true)
    {
        pair = next(in);
        if (pair.first == NULL)
        {
            if (pair.second == -1)
            {
                return 1;
            }
            if (pair.second == 0)
            {
                break;
            }
        }
        int count = count_of_null(pair.first, pair.second);
        if (count < 3)
        {
            return 2;
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
    for (int i = 0; i < pids.size(); i++)
    {
        int st;
        waitpid(pids[i], &st, 0);
    }
    close(in);
    return 0;
}
