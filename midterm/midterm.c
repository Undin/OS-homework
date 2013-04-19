#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <sys/wait.h>
#include <map>
#include <fcntl.h>

int eof = 0;

int buffer_size = 4096;
char buffer[4096];
char str[4096];
int read_symbol = 0;
std::vector<char *> values;
std::vector<char *> keys;

int next(char *buffer, int begin, int end)
{
    int i;
    for (i = begin; i != end; i++)
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
    int i = 0;
    for (i = 0; i < len; i++)
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


std::multimap<char *, char *, bool(*)(char*, char *)> m(comp);

int main(int arc, char *argv[])
{
    while (!eof)
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
                eof = 1;
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
            char *str = (char *)malloc(sizeof(char) * (pos + 2));
            memcpy(str, buffer, pos + 1);
            str[pos + 1] = '\0';
            values.push_back(str);
            memmove(buffer, buffer + pos + 1, sizeof(char) * (buffer_size - pos - 1));
            read_symbol -= pos + 1;
        }
    }

    
    int pid;
    int i;
    char **margv = (char **)malloc(sizeof(char *) * 2);
    char **echo = (char **)malloc(sizeof(char *) * 3);
    char e[] = "echo";
    char s[] = "cat";
    //char f[] = "c";
    margv[0] = s;
    //margv[1] = f;
    margv[1] = NULL;
    echo[0] = e;
    echo[2] = NULL;
    int out = dup(1);
    for (i = 0; i < values.size(); i++)
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
            int position;
            while (res)
            {
                res = read(0, buffer + symbol, 1);
                //printf("%d\n", symbol);
                symbol += res;
            }
            
            //printf("%d\n", symbol);
            /*if (symbol == buffer_size)
            {
                exit(1);
            }*/
            char *ss = (char *)malloc(sizeof(char) * (symbol + 1));
            //scanf("%s", ss);
            memcpy(ss, buffer, symbol);
            ss[symbol] = '\0';
            char *j;
            for (j = ss; *j != '\0'; j++)
            {
                if (*j == '\\')
                {
                    *j = '|';
                }
                if (*j == '\n')
                {
                    *j = '#';
                }
            }

            //printf("!!!%s?", ss);

            std::multimap<char *, char *, bool(*)(char *, char *)>::iterator it = m.find(ss);
            if (it == m.end())
            {
                keys.push_back(ss);
                //printf("YA VSTAVILOS%s\n", keys.back());
                m.insert(std::make_pair(ss, values[i]));
            }
            else
            {
                m.insert(std::make_pair((*it).first, values[i]));
                free(ss);
            }
            //scanf("%s", ss);
            //free(ss);
        }
        else
        {
            int fds2[2];
            pipe(fds2);
            int pid2;
            if (pid2 = fork())
            { 
                //printf("!!!\n");
                dup2(fds2[1], 1);
                close(fds[0]);
                close(fds[1]);
                close(fds2[1]);
                close(fds2[0]);
                echo[1] = values[i];
                execvp(echo[0], echo);
                /*                int stat;
                waitpid(pid2, &stat, 0);
                return 0;*/
            }
            else
            {   
                //printf("&&&\n");
                dup2(fds2[0], 0);
                dup2(fds[1], 1);
                close(fds[0]);
                close(fds[1]);
                close(fds2[0]);
                close(fds2[1]);
                execvp(margv[0], margv);
            }
        }
    }
    std::multimap<char *, char *, bool(*)(char *, char *)>::iterator iter;
    /*for (iter = m.begin(); iter != m.end(); iter++)
    {
        printf("|%s|\n", iter->first);
    }*/
    for (i = 0; i < keys.size(); i++)
    {
        //printf("key = %s\n", keys[i]);
        std::pair<std::multimap<char *, char *, bool(*)(char *, char *)>::iterator, 
                  std::multimap<char *, char *, bool(*)(char *, char *)>::iterator > result = m.equal_range(keys[i]);
        //printf("^^^%d^^^\n", result.first == result.second);
        int fd = open(keys[i], O_CREAT, 0666);
        close(fd);
        fd = open(keys[i], O_WRONLY);
        std::multimap<char *, char *, bool(*)(char *, char *)>::iterator it = result.first;
        for (it = result.first; it != result.second; it++)
        {
            //printf("$$$%s\n", it->second);
            print(fd, it->second, strlen(it->second));
        }
        close(fd);
    }

    for (i = 0; i < values.size(); i++)
    {
        free(values[i]);
    }
    for (i = 0; i < keys.size(); i++)
    {
        free(keys[i]);
    }

    return 0;
}

