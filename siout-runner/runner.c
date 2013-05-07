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
       int res = read(fd, buffer + position, buffer_size - position);
       if (res == 0)
       {
           eof = true;
       }
       position += res;
   }
   return make_pair(char *(0), 0);
}


int main(int argc, char **argv)
{
    int in = open(argv, O_RDONLY);
    
    return 0;
}
