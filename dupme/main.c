#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char *buffer;
int buffer_size;

void print(const char *buf, size_t count, size_t times)
{
    size_t written;
    size_t i;
    for (i = 0; i < times; ++i)
    {
        written = 0;
        while (written < count)
        {
            written += write(1, buf + written, count - written);
        }
    }
}

int main(int argc, char *argv[])
{
    return 0;
}
