#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char *buffer;
char *output_buffer;
int buffer_size;

void print(int fd, const char *buf, size_t count, size_t times)
{
    size_t written;
    size_t i;
    buf[count] = '\n';
    count++;
    for (i = 0; i < times; ++i)
    {
        written = 0;
        while (written < count)
        {
            written += write(fd, buf + written, count - written);
        }
    }
}

int check_end(const char *buf, char *out_buf, int begin, int end)
{
    int i;
    for (i = begin; i < end; i++)
    {
        if (buf[i] == '\n')
        {
            if (out_buf != NULL)
            {
                memcpy(out_buf, buf, i * sizeof(char));
            }
            memmove(buf, buf + i + 1, (end - i - 1) * sizeof(char));
            return i + 1;
        }
    }
    return 0;

int next_token(int fd, char *buf, char *out_buf, size_t size)
{
    static int pos = 0;
    static int count = 0;
    static int max_size = size;
    static int eof = 0;
    int long_string = 0;
    int read_res;
    int res = check_end(buf, out_buf, count, pos);
    if (res != 0)
    {
        count = 0;
        pos -= res;
        if (pos == 0)
        {
            max_size = size;
        }
        return res - 1;
    }
    if (pos != 0)
    {
        max_size++;
    }
    count = pos;
    if (eof == 1)
    {
        return 0;
    }
    while (0 == 0)
    {
        while (pos != max_size && long_string == 1)
        {
            read_res = read(fd, buf + pos, max_size - pos);
            if (read_res == 0)
            {
                eof = 1;
                return 0;
            }
            pos += read_res;
            res = check_end(buf, NULL, count, pos);
            if (res != 0)
            {
                count = 0;
                pos -= res;
                long_string = 0;
            }
            if (pos == 0)
            {
                max_size = size;
            }
            else
            {
                max_size++;
            }
            count = pos;
        }
        if (long_string == 0)
        {   
            while (pos != max_size)
            {
                read_res = read(fd, buf + pos, max_size - pos);
                if (read_res == 0)
                {
                    eof = 1;
                }
                pos += read_res;
                res = check_end(buf, out_buf, count, pos);
                if (res != 0)
                {
                    count = 0;
                    pos -= res;
                    if (pos == 0)
                    {
                        max_size = size;
                    }
                    return res - 1;
                }
                count = pos;
            }
        }
        long_string = 1;
        count = 0;
        pos = 0;
        max_size = size;
    }
}

int main(int argc, char *argv[])
{
    int k = 0;

    return 0;
}
