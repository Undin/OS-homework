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
const char OK[] = "ok";
const char ERR[] = "err ";
const char delimiter = '\0';

int pid;
bool end_file = false;

void handler(int)
{
    kill(pid, SIGINT);
}

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

int mread(int fd, char *buffer, int size, const char *del = NULL)
{
    int res = 1;
    int readed = 0;
    int find_del;
    while (res != 0 && readed < size)
    {
        res = read(fd, buffer + readed, size - readed);
                if (res == -1)
        {
            return -1;
        }

        if (del != NULL)
        {
            find_del = find_delimiter(buffer, readed, res, *del);
            if (find_del != -1)
            {
                return find_del;
            }
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

int main()
{
    pid = fork();
    if (pid == 0)
    {
        setsid();
        printf("create daemon\n");
        struct addrinfo hints;
        struct addrinfo *result;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        int s = getaddrinfo(NULL, "8822", &hints, &result);
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
        int yes = 1;
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0)
        {
            perror("error int setsockopt");
            exit(1);
        }

        if (bind(sfd, result->ai_addr, result->ai_addrlen) != 0)
        {
            perror("error in bind");
            exit(1);
        }
        if (listen(sfd, 5) != 0)
        {
            perror("error in listen");
            exit(1);
        }
        struct sockaddr addr;
        socklen_t addr_len = sizeof(struct sockaddr);
        while (true)
        {
            int fd = accept(sfd, &addr, &addr_len);
            printf("accept\n");
            if (fd == -1)
            {
                perror("error in accept");
                exit(1);
            }
            if (fork() != 0)
            {
                close(fd);
            }
            else
            {
                dup2(fd, 2);
                dup2(fd, 1);
                dup2(fd, 0);
                close(fd);
                char file_buffer[buffer_size];
                int file_size = mread(0, file_buffer, buffer_size, &delimiter);
                if (file_size <= 0 || file_buffer[file_size - 1] != delimiter)
                {
                    perror("error in filename reading");
                    exit(1);
                }
                int file_fd = open(file_buffer, O_RDONLY);
                if (file_fd == -1)
                {
                    print(1, ERR, 4);
                    perror("\0");
                    exit(0);
                }
                print(1, OK, 2);
                struct stat st;
                fstat(file_fd, &st);
                std::string number = std::to_string(st.st_size);
                print(1, number.c_str(), strlen(number.c_str()));
                print(1, "\0", 1);
                char buffer[buffer_size];
                int res = 1;
                while (res > 0)
                {
                    res = mread(file_fd, buffer, buffer_size);
                    if (res != -1)
                    {
                        print(1, buffer, res);
                    }
                }
                close(file_fd);
                exit(0);
            }
        }
        close(sfd);
    }
    else
    {
        signal(SIGINT, handler);
        int status;
        waitpid(pid, &status, 0);
    }    
    return 0;
}
