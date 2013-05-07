#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <utility>

std::vector<std::pair<std::pair<char *, char*>, std::vector<char *>> data;
std::vector<char *> strings;
char buffer[4096];
int buffer_size = 4096;
int position = 0;
int current_position = 0;
bool eof = false;

std::pair<char *, int> next(int fd)
{
   int i;
   while (!eof)
   {
       for (i = current_position; i < position - 1; i++)
       {
           if (buffer_size[i] == '\0' && buffer_size[i + 1] == '\0')
           {
               char *p = malloc(i + 1);
               memcpy(p, buffer, i + 1);
               memmove(buffer, buffer + i + 2, position - i - 2);
               position -= i + 2;
               current_position = 0;
               return std::make_pair(p, i + 2);
           }
       }
       if (position == buffer_size)
       {
           return std::make_pair(char*(0), -1);
       }
       int res = read(fd, buffer + position, buffer_size - position);
       if (res == 0)
       {
           eof = true;
       }
       position += res;
   }
   return make_pair(char *(0), 0);
}

int number_of_null(char *p, int n)
{
    char *i;
    int count = 0;
    for (i = p; i < p + n, i++)
    {
        if (*i == '\0')
        {
            count++;
        }
    }
    return count;
}

void execution(char *p, int n, int l)
{
    char *s1, *s2;
    int in = open(p, O_RDONLY);
    for (s1 = p + n - 2; s1 != '\0'; s1--);
    int out = open(s1 + 1, O_WRONLY | O_CREAT, 0644);
    int len = strlen(p) + 1;
    char **arg = malloc(l + 1);
    int i;
    for (s2 = p + len, i = 0; i < l - 1; s2 += strlen(s2) + 1, i++)
    {
        arg[i] = s2;
    }
    arg[l - 1] = NULL;
    dup2(in, 0);
    dup2(out, 1);
    execvp(arg[0], arg);
}

int main(int argc, char **argv)
{
    int in = open(argv, O_RDONLY);
    std::pair<char *, int> pair;
    while (true)
    {
        pair = next(in);
        if (pair.first == NULL)
        {
            if (pair.second == -1)
            {
                return -1;
            }
            if (pair.second == 0)
            {
                break;
            }
        }

        int count = count_of_null(pair.first, pair.second);
        if (count < 3)
        {
            return -2;
        }
        if (!fork())
        {
            execution(pair.first, pair.second, count - 2);
        }
    }
    return 0;
}
