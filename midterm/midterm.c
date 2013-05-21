#include <unistd.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <sys/wait.h>
#include <map>
#include <fcntl.h>

static const int buffer_size = 4096;

int next(char *buffer, int begin, int end)
{
    for (int i = begin; i != end; i++)
    {
        if (buffer[i] == '\n')
        {
            return i;
        }
    }
    return -1;
}

bool comp(char* a, char* b)
{
    size_t len = strlen(a);
    size_t len_b = strlen(b);
    if (len_b < len)
    {
        len = len_b;
    }
    for (size_t i = 0; i < len; i++)
    {
        if (a[i] != b[i])
        {
            return a[i] < b[i];
        }
    }
    return false;
}

void print(int fd, char *buffer, int len)
{
    int written = 0;
    while (len - written > 0)
    {
        written += write(fd, buffer + written, len  - written);
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
    std::vector<char *> values;
    int read_symbol = 0;
    int eof_flag = 0;
    char str[buffer_size];
    char buffer[buffer_size];

    while (eof_flag == 0)
    {
        int position = 0;
        int pos;
        while ((pos = next(buffer, position, read_symbol)) == -1)
        {
            position = read_symbol;
            if (read_symbol == buffer_size)
            {
                return -1;
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
                buffer[read_symbol++] = '\n';
            }
            read_symbol += res;
        }
        if (pos != -1)
        {
            char *str = (char *) safe_malloc(pos + 2);
            memcpy(str, buffer, pos + 1);
            str[pos + 1] = '\0';
            values.push_back(str);
            memmove(buffer, buffer + pos + 1, buffer_size - pos - 1);
            read_symbol -= pos + 1;
        }
    }
    
    char **margv = (char **) safe_malloc(sizeof(char *) * argc);
    for (int j = 0; j < argc - 1; j++)
    {
        margv[j] = argv[j + 1];
    }
    margv[argc - 1] = NULL;
    std::multimap<char *, char *, bool(*)(char*, char *)> m(comp);
    int pid;
    std::vector<char *> keys;
    for (size_t i = 0; i < values.size(); i++)
    {
        int fds[2];
        pipe(fds);
        if ((pid = fork()))
        {
            dup2(fds[0], 0);
            close(fds[0]);
            close(fds[1]);
            
            int stat;
            waitpid(pid, &stat, 0);

            int res = 1;
            int symbol = 0;
            while (res)
            {
                res = read(0, buffer + symbol, 1);
                symbol += res;
            }
            
            if (symbol == buffer_size)
            {
                exit(1);
            }
            char *ss = (char *) safe_malloc(symbol + 1);
            memcpy(ss, buffer, symbol);
            ss[symbol] = '\0';
            for (char *j = ss; *j != '\0'; j++)
            {
                if (*j == '\\')
                {
                    *j = '|';
                }
            }

            std::multimap<char *, char *, bool(*)(char *, char *)>::iterator it = m.find(ss);
            if (it == m.end())
            {
                keys.push_back(ss);
                m.insert(std::make_pair(ss, values[i]));
            }
            else
            {
                m.insert(std::make_pair((*it).first, values[i]));
                free(ss);
            }
        }
        else
        {
            int fds2[2];
            pipe(fds2);
            int pid2;
            if (!(pid2 = fork()))
            { 
                dup2(fds2[1], 1);
                close(fds[0]);
                close(fds[1]);
                close(fds2[1]);
                close(fds2[0]);
                print(1, values[i], strlen(values[i]));
                return 0;
            }
            else
            {   
                dup2(fds2[0], 0);
                dup2(fds[1], 1);
                close(fds[0]);
                close(fds[1]);
                close(fds2[0]);
                close(fds2[1]);
                int stat;
                waitpid(pid2, &stat, 0);
                execvp(margv[0], margv);
            }
        }
    }
    std::multimap<char *, char *, bool(*)(char *, char *)>::iterator iter;
    for (size_t i = 0; i < keys.size(); i++)
    {
        std::pair<std::multimap<char *, char *, bool(*)(char *, char *)>::iterator, 
                  std::multimap<char *, char *, bool(*)(char *, char *)>::iterator> result = m.equal_range(keys[i]);
        int l = strlen(keys[i]);
        memcpy(str, keys[i], l - 1);
        str[l - 1] = '\0';
        int fd = open(str, O_CREAT, 0666);
        close(fd);
        fd = open(str, O_WRONLY);
        for (std::multimap<char *, char *, bool(*)(char *, char *)>::iterator it = result.first; it != result.second; it++)
        {
            print(fd, it->second, strlen(it->second));
        }
        close(fd);
    }

    for (size_t i = 0; i < values.size(); i++)
    {
        free(values[i]);
    }
    for (size_t i = 0; i < keys.size(); i++)
    {
        free(keys[i]);
    }
    return 0;
}
