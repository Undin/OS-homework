#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void print(int fd, char *buf, size_t count, size_t times)
{
    if (count == 0 || buf[count - 1] != '\n')
    {
        buf[count] = '\n';
        count++;
    }
    size_t written;
    size_t i;
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
                memcpy(out_buf, buf, i);
            }
            memmove(buf, buf + i + 1, end - i - 1);
            return i + 1;
        }
    }
    return -1;
}

int next_token(int fd, char *buf, char *out_buf, int size)
{
    static int pos = 0;
    static int count = 0;
    static int eof_flag = 0;
    int long_string = 0;
    int read_res;
    if (size == 0 || eof_flag == 1)
    {
        return -1;
    }
    int res = check_end(buf, out_buf, count, pos);
    if (res != -1)
    {
        count = 0;
        pos -= res;
        return res - 1;
    }
    count = pos;
    while (1)
    {
        while (pos != size && long_string == 1)
        {
            read_res = read(fd, buf + pos, size - pos);
            if (read_res == 0)
            {
                eof_flag = 1;
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
            count = pos;
         }
        if (long_string == 0)
        {   
            while (pos != size)
            {
                read_res = read(fd, buf + pos, size - pos);
                if (read_res == 0)
                {
                    eof_flag = 1;
                    if (pos != 0)
                    {
                        memcpy(out_buf, buf, pos);
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
                    return res - 1;
                }
                count = pos;
            }
        }
        long_string = 1;
        count = 0;
        pos = 0;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }
    int buffer_size = atoi(argv[1]);
    char *buffer = malloc(buffer_size + 1);
    char *output_buffer = malloc(buffer_size + 1);
    int count;
    while ((count = next_token(0, buffer, output_buffer, buffer_size + 1)) >= 0)
    {
        print(1, output_buffer, count, 2);
    }
    free(buffer);
    free(output_buffer);
    return 0;
}
