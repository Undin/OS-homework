#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main()
{   
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
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
        printf("Hello\n");
        exit(0);
    }
    close(sfd);    
    return 0;
}
