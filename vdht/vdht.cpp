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
#include <vector>
#include <utility>
#include <string>
#include <set>
#include <map>

using namespace std;

const int ERR = POLLERR | POLLHUP | POLLNVAL;
const size_t BUFFER_SIZE = 4096;
const string COLLISION = "COLLISION";

void add_message(char *buffer, size_t *pos, const string &message)
{
    if (BUFFER_SIZE - *pos >= message.size())
    {
        memcpy(buffer + *pos, message.c_str(), message.size());
        *pos += message.size();
    }
}

void print(int fd, std::pair<char *, size_t> *buf)
{
    int res = write(fd, buf->first, buf->second);
    if (res > 0)
    {
        buf->second -= res;
        memmove(buf->first, buf->first + res, buf->second);
    }
}

int next_token(char *buffer, int  begin, int end, char delimiter)
{
    for (int i = begin; i < end; i++)
    {
        if (buffer[i] == delimiter)
        {
            return i + 1;
        }
    }
    return -1;
}

string next(string &str, char delimiter)
{
    int pos = str.find(delimiter);
    string s(str, 0, pos);
    str.erase(0, pos + 1);
    return s;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        perror("invalid arguments");
        exit(1);
    }
    
    vector<pair<string, string> > address;
    for (int i = 2; i < argc; i++)
    {
        string str(argv[i]);
        char *pos = strchr(argv[i], ':');
        address.push_back(make_pair(string(argv[i], pos), string(pos + 1)));
    }

    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, argv[1], &hints, &result);
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
    if (listen(sfd, 10) != 0)
    {
        perror("error in listen");
        exit(1);
    }
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    
    vector<pollfd> fds;
    vector<pair<char *, size_t> > buffers;
    vector<size_t> readed;
    map<string, vector<string> > m;
    set<string> messages;

    pollfd p;

    p.events = POLLIN | ERR;
    p.fd = sfd;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)NULL, 0));
    readed.push_back(0);

    p.events = POLLIN | ERR;
    p.fd = 0;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
    readed.push_back(0);

    p.events = 0;
    p.fd = 2;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
    readed.push_back(0);

    for (size_t i = 0; i < address.size(); i++)
    {
        hints.ai_flags = 0;
        int s = getaddrinfo(address[i].first.c_str(), address[i].second.c_str(), &hints, &result);
        if (s == 0)
        {
            int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (fd != -1)
            {
                if (connect(fd, result->ai_addr, result->ai_addrlen) != -1)
                {
                    printf("connect %s %s \n", address[i].first.c_str(), address[i].second.c_str());
                    p.fd = fd;
                    p.events = POLLIN | ERR;
                    fds.push_back(p);
                    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
                    readed.push_back(0);

                    p.events = 0;
                    fds.push_back(p);
                    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
                    readed.push_back(0);
                }
            }
        }
    }

    while (true)
    {
        int res = poll(fds.data(), fds.size(), -1);
        if (res == -1)
        {
            printf("(((((\n");
            break;
        }
        if (fds[0].revents & ERR)
        {
            fds[0].events = 0;
        }
        if (fds[0].revents & POLLIN)
        {
            int fd = accept(sfd, &addr, &addr_len);
            p.fd = fd;

            p.events = POLLIN | ERR;
            fds.push_back(p);
            buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
            readed.push_back(0);

            p.events = 0;
            fds.push_back(p);
            buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
            readed.push_back(0);

            printf("new client connected\n");
        }

        for (size_t i = 1; i < fds.size(); i++)
        {
            if (fds[i].revents & ERR)
            {
                fds[2 * ((i + 1) / 2) - 1].events = 0;
                fds[2 * ((i + 1) / 2)].events = 0;
            }

            if (buffers[i].second != BUFFER_SIZE && fds[i].revents & POLLIN)
            {
                int r = read(fds[i].fd, buffers[i].first + buffers[i].second, BUFFER_SIZE - buffers[i].second);
                if (r >= 0)
                {
                    buffers[i].second += r;
                }
                else
                {
                    fds[i].events = 0;
                    fds[i + 1].events = 0;
                }
            }

            if (fds[i].revents & POLLOUT)
            {
                print(fds[i].fd, &buffers[i]);
                if (buffers[i].second == 0)
                {
                    fds[i].events = 0;
                }
            }
        }

        if (buffers[1].second != 0 && buffers[1].second > readed[1])
        {
            int res = next_token(buffers[1].first, readed[1], buffers[1].second, '\n');
            if (res == -1)
            {
                readed[1] = buffers[1].second;
            }
            else
            {
                string command(buffers[1].first, res);
                buffers[1].second -= res;
                readed[1] = 0;
                memmove(buffers[1].first, buffers[1].first + res, buffers[1].second);

                string com = next(command, ' ');
                if (com == "a")
                {
                    string key = next(command, ' ');
                    string value1 = next(command, ' ');
                    string value2 = next(command, '\n');
                    string id(argv[1]);
                    id.append(to_string(time(NULL)));
                    messages.insert(id);
                    string message;
                    bool is_insert = true;
                    auto it = m.find(key);
                    if (it == m.end())
                    {
                        //vector<string> tmp_v = {value1};
                        m[key] = vector<string>();
                        it = m.find(key);
                        is_insert = false;
                    }
                    else
                    {
                        for (int j = it->second.size() - 1; j >= 0; j--)
                        {
                            if (it->second[j] == value1)
                            {
                                if ((size_t)j != it->second.size() - 1)
                                {
                                    if (it->second.back() != COLLISION)
                                    {
                                        it->second.push_back(COLLISION);
                                    }
                                    message = "c ";
                                    message.append(id + " ");
                                    message.append(key + "\n");
                                }
                                else
                                {
                                    is_insert = false;
                                }
                                break;
                            }
                        }
                        
                    }
                    if (!is_insert)
                    {
                        it->second.push_back(value2);
                        message = "a ";
                        message.append(id + " ");
                        message.append(key + " ");
                        message.append(value1 + " ");
                        message.append(value2 + "\n");
                    }
                    if (message.size() > 0)
                    {
                        for (size_t j = 4; j < fds.size(); j += 2)
                        {
                            add_message(buffers[j].first, &buffers[j].second, message);
                            fds[j].events = POLLOUT | ERR;
                        }
                    }
                }
                if (com == "p")
                {
                    string key = next(command, '\n');
                    auto it = m.find(key);
                    if (it != m.end())
                    {
                        string message = it->second[0];
                        for (size_t j = 1; j < it->second.size(); j++)
                        {
                            message.append("->" + it->second[j]);
                        }
                        message += "\n";
                        add_message(buffers[2].first, &buffers[2].second, message);
                        fds[2].events = POLLOUT | ERR;
                    }
                }
            }
        }
        
        for (size_t i = 3; i < fds.size(); i += 2)
        {
        
            if (buffers[i].second != 0 && buffers[i].second > readed[i])
            {
                int res = next_token(buffers[i].first, readed[i], buffers[i].second, '\n');
                if (res == -1)
                {
                    readed[i] = buffers[i].second;
                }
                else
                {
                    string command(buffers[i].first, res);
                    string command2 = command;
                    buffers[i].second -= res;
                    readed[i] = 0;
                    memmove(buffers[i].first, buffers[i].first + res, buffers[i].second);
                    string com = next(command, ' ');
                    string id = next(command, ' ');
                    string message;
                    bool is_insert = true;
                    if (messages.find(id) == messages.end())
                    {
                        messages.insert(id);
                        if (com == "a")
                        {
                            string key = next(command, ' ');
                            string value1 = next(command, ' ');
                            string value2 = next(command, '\n');
                            auto it = m.find(key);
                            if (it == m.end())
                            {
                                //vector<string> tmp_v = {value1};
                                m[key] = vector<string>();
                                it = m.find(key);
                                is_insert = false;
                            }
                            else
                            {
                                for (int j = it->second.size() - 1; j >= 0; j--)
                                {
                                    if (it->second[j] == value1)
                                    {
                                        if ((size_t)j != it->second.size() - 1)
                                        {
                                            if (it->second.back() != COLLISION)
                                            {
                                                it->second.push_back(COLLISION);
                                            }
                                            id = string(argv[1]);
                                            id.append(to_string(time(NULL)));
                                            message = "c ";                                                                          message.append(id + " ");
                                            message.append(key + "\n");
                                            messages.insert(id);
                                        }
                                        else
                                        {
                                            is_insert = false;
                                        }
                                        break;
                                    }
                                }
                            }
                            if (!is_insert)
                            {
                                it->second.push_back(value2);
                                message = command2;
                            }
                        }
                        if (com == "c")
                        {
                            //printf("мне пришла коллизия!\n");
                            message = command2;
                            string key = next(command, '\n');
                            auto it = m.find(key);
                            if (it == m.end())
                            {
                                //printf("???\n");
                                vector<string> tmp_v = {COLLISION};
                                m[key] = tmp_v;
                            }
                            else
                            {
                                //printf("!!!\n");
                                if (it->second.back() != COLLISION)
                                {
                                    it->second.push_back(COLLISION);
                                }
                            }
                        }
                        if (message.size() > 0)
                        {
                            for (size_t j = 4; j < fds.size(); j += 2)
                            {
                                add_message(buffers[j].first, &buffers[j].second, message);
                                fds[j].events = POLLOUT | ERR;
                            }
                        }
                    }
                }
            }
        }
    }
    
    close(sfd);
    return 0;
}
