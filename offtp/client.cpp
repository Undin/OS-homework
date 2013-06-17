#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pty.h>
#include <sys/wait.h>
#include <poll.h>
#include <errno.h>
#include <string>

const int buffer_size = 4096;
const char delimiter = '\0';

int find_delimiter(char *buffer, int readed, int res, char del)
{
    for (int i = 0; i < res; i++)
    {
        if (buffer[readed + i] == del)
        {
            return readed + i + 1;
        }
    }
    return -1;
}

int mread(int fd, char *buffer, int size)
{
    int res = 1;
    int readed = 0;
    while (res != 0 && readed < size)
    {
        res = read(fd, buffer + readed, size - readed);
        if (res == -1)
        {
            return -1;
        }
        readed += res;
    }
    return readed;
}

void print(int fd, const char *buffer, int size)
{
    int written = 0;
    while (size - written > 0)
    {
        written += write(fd, buffer + written, size - written);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        perror("illegal number of argument");
        exit(1);
    }
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int s = getaddrinfo("127.0.0.1", "8822", &hints, &result);
    if (s != 0)
    {
        perror("error in getaddrinfo");
        exit(1);
    }
    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd == -1)
    {
        perror("error in socket");
        exit(1);
    }
    if (connect(sfd, result->ai_addr, result->ai_addrlen) == -1)
    {
        perror("error in connect");
        exit(1);
    }
    print(sfd, argv[1], strlen(argv[1]) + 1);
    char buffer[buffer_size];
    int res = mread(sfd, buffer, buffer_size);
    if (res == -1)
    {
        perror("error in reading");
        exit(1);
    }
    int del_pos = find_delimiter(buffer, 0, res, delimiter);
    if (del_pos == -1)
    {
        perror("error in reading");
        exit(1);
    }
    print(2, buffer, del_pos);
    if (buffer[0] == 'e')
    {
        return 0;
    }
    int symbols = atoi(buffer + 2);
    int number_of_symbol = 0;
    //int q = open("q", O_WRONLY | O_CREAT, 0666);
    memmove(buffer, buffer + del_pos, buffer_size - del_pos);
    print(1, buffer, res - del_pos);
    number_of_symbol += res - del_pos;
    while (res > 0)
    {
        res = mread(sfd, buffer, buffer_size);
        if (res != -1)
        {
            print(1, buffer, res);
            number_of_symbol += res;
        }
    }

    //printf("number_of_symbol=%d\n", number_of_symbol);
    //printf("symbols=%d\n", symbols);
    if (number_of_symbol == symbols)
    {
        print(2, "all ok\n", 7);
    }
    else
    {
        print(2, "bad file\n", 9);
    }
    return 0;
}

