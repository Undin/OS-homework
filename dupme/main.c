#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void print(int fd, char *buf, size_t count, size_t times)
{
    size_t written;
    size_t i;
    if (count == 0 || buf[count - 1] != '\n')
    {
        buf[count] = '\n';
        count++;
    }
    for (i = 0; i < times; ++i)
    {
        written = 0;
        while (written < count)
        {
            written += write(fd, buf + written, count - written);
        }
    }
}

int check_end(char *buf, char *out_buf, int begin, int end)
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
    return -1;
}

int next_token(int fd, char *buf, char *out_buf, size_t size)
{
    static int pos = 0;
    static int count = 0;
    static int max_size = 0;
    static int eof = 0;
    int long_string = 0;
    int read_res;
    if (size == 0 || eof == 1)
    {
        return -1;
    }
    if (pos == 0)
    {
        max_size = size;
    }
    else
    {
        max_size = size + 1;
    }
    int res = check_end(buf, out_buf, count, pos);
    if (res != -1)
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
        max_size = size + 1;
    }
    count = pos;
    while (0 == 0)
    {
        while (pos != max_size && long_string == 1)
        {
            read_res = read(fd, buf + pos, max_size - pos);
            if (read_res == 0)
            {
                eof = 1;
                return -1;
            }
            pos += read_res;
            res = check_end(buf, NULL, count, pos);
            if (res != -1)
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
                max_size = size + 1;
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
                    if (pos != 0)
                    {
                        memcpy(out_buf, buf, pos * sizeof(char));
                        return pos;
                    }
                    return -1;
                }
                pos += read_res;
                res = check_end(buf, out_buf, count, pos);
                if (res != -1)
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
                    max_size = size + 1;
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

char *buffer;
char *output_buffer;

int main(int argc, char *argv[])
{
    int buffer_size = 0;
    int i;
    if (argc == 1)
    {
        return 1;
    }
    for (i = 0; argv[1][i] != 0; ++i)
    {
        buffer_size *= 10;
        buffer_size += argv[1][0] - '0';
    }
    buffer = malloc((buffer_size + 1) * sizeof(char));
    output_buffer = malloc((buffer_size + 1) * sizeof(char));
    int count;
    while ((count = next_token(0, buffer, output_buffer, buffer_size)) >= 0)
    {
        print(1, output_buffer, count, 2);
    }
    free(buffer);
    free(output_buffer);
    return 0;
}
