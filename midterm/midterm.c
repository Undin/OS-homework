#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>

int eof = 0;

int buffer_size = 4096;
char buffer[4096];
int read_symbol = 0;
std::vector<char *> v;

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
            v.push_back(str);
            memmove(buffer, buffer + pos + 1, sizeof(char) * (buffer_size - pos - 1));
            read_symbol -= pos + 1;
        }
    }

    for (int i = 0; i < v.size(); i++)
    {
        printf("%s", v[i]);
        free(v[i]);
    }
    return 0;
}

