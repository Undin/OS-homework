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

int pid;

void handler(int)
{
    kill(pid, SIGINT);
}

void print(int fd, char *buf, int size)
{
    int written = 0;
    while (written < size)
    {
        written += write(fd, buf + written, size - written);
    }
}

bool exchange_data(int fd1, int fd2, char *buf, int size)
{
    int res = read(fd1, buf, size);
    if (res > 0)
    {
        print(fd2, buf, res);
        return true;
    }
    return false;
}

int main()
{
    pid = fork();
    if (!pid)
    {
        setsid();
        printf("create session 1\n");
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
                int master, slave;
                char buffer[4096];
                int ttyfd = openpty(&master, &slave, buffer, NULL, NULL);
                if (ttyfd == -1)
                {
                    perror("error in openpty");
                    exit(1);
                }
                if (!fork())
                {
                    close(fd);
                    close(master);
                    setsid();
                    printf("create session 2\n");
                    int ff = open(buffer, O_RDWR);
                    close(ff);
                    dup2(slave, 0);
                    dup2(slave, 1);
                    dup2(slave, 2);
                    close(slave);
                    execl("/bin/bash", "bash", NULL);
                    exit(255);
                }
                else
                {
                    close(slave);
                    fcntl(master, O_NONBLOCK);
                    fcntl(fd, O_NONBLOCK);
                    char buf[4096];
                    while (true)
                    {
                        if (!(exchange_data(master, fd, buf, 4096) &&
                              exchange_data(fd, master, buf, 4096)))
                        {
                            break;
                        }
                        sleep(1);
                    }
                    close(master);
                    close(fd);
                    exit(0);
                }
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
