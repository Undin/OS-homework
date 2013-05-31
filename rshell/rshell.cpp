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

int pid;


void handler(int)
{
    kill(pid, SIGINT);
}

const int ERR = POLLERR | POLLHUP | POLLNVAL;
const int buffer_size = 4096;

void close_and_exit(int fd1, int fd2)
{
    close(fd1);
    close(fd2);
    exit(1);
}

int main()
{
    pid = fork();
    if (pid == 0)
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
                char buffer[buffer_size];
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
                    execl("/bin/sh", "sh", NULL);
                    exit(255);
                }
                else
                {
                    close(slave);
                    char buf[2][buffer_size];
                    int size[2];
                    bool dead[2];
                    dead[0] = false;
                    dead[1] = false;
                    size[0] = 0;
                    size[1] = 0;
                    pollfd fds[2];
                    fds[0].fd = master;
                    fds[1].fd = fd;
                    fds[0].events = POLLIN | ERR;
                    fds[1].events = POLLIN | ERR;
                    while (true)
                    {
                        int res = poll(fds, 2, -1);
                        if (res == -1)
                        {
                            if (errno == EINTR)
                            {
                                continue;
                            }
                            else
                            {
                                close_and_exit(fd, master);
                            }
                        }
                        for (int i = 0; i < 2; i++)
                        {
                            if (!dead[i] && fds[i].revents & ERR)
                            {
                                dead[i] = true;
                            }
                            if (!dead[i] && fds[i].revents & POLLIN)
                            {
                                int res = read(fds[i].fd, buf[i] + size[i], buffer_size - size[i]);
                                if (res <= 0)
                                {
                                    dead[i] = true;
                                }
                                else
                                {
                                    size[i] += res;
                                }
                            }
                            if (!dead[i] && fds[i].revents & POLLOUT)
                            {
                                int r = write(fds[i].fd, buf[1 - i], size[1 - i]);
                                if (r < 0)
                                {
                                    dead[i] = true;
                                }
                                else
                                {

                                    memmove(buf[1 - i], buf[1 - i] + r, size[1 - i] - r);
                                    size[1 - i] -= r;
                                }
                            }
                        }
                        if (dead[0] && dead[1])
                        {
                            close_and_exit(fd, master);
                        }
                        for (int i = 0; i < 2; i++)
                        {
                            if (size[i] > 0)
                            {
                                fds[1 - i].events |= POLLOUT;
                                if (size[i] == buffer_size)
                                {
                                    fds[i].events &= ~POLLIN;
                                }
                                else
                                {
                                    fds[i].events |= POLLIN;
                                }
                            }
                        }
                        if ((dead[0] && size[0] == 0) || (dead[1] && size[1] == 0))
                        {
                            close_and_exit(fd, master);
                        }
                    }
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
