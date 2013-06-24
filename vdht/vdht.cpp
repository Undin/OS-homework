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

int pid;

void handler(int)
{
    kill(pid, SIGINT);
}

const int ERR = POLLERR | POLLHUP | POLLNVAL;
const size_t BUFFER_SIZE = 4096;
const string COLLISION = "COLLISION";

enum class state
{
    ALIVE,
    DEAD
};

void add_message(char *buffer, size_t *pos, const string &message)
{
    if (BUFFER_SIZE - *pos >= message.size())
    {
        memcpy(buffer + *pos, message.c_str(), message.size());
        *pos += message.size();
    }
}

bool print(int fd, std::pair<char *, size_t> *buf)
{
    int res = write(fd, buf->first, buf->second);
    if (res < 0)
    {
        return false;
    }
    buf->second -= res;
    memmove(buf->first, buf->first + res, buf->second);
    return true;
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
    vector<state> st;
    vector<size_t> readed;
    map<string, pair<set<string>, vector<string> > > m;

    pollfd p;

    p.events = POLLIN | ERR;
    p.fd = sfd;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)NULL, 0));
    st.push_back(state::ALIVE);
    readed.push_back(0);

    p.events = POLLIN | ERR;
    p.fd = 0;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
    st.push_back(state::ALIVE);
    readed.push_back(0);

    p.events = POLLOUT | ERR;
    p.fd = 2;
    fds.push_back(p);
    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
    st.push_back(state::ALIVE);
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
                    st.push_back(state::ALIVE);
                    readed.push_back(0);

                    p.events = POLLOUT | ERR;
                    fds.push_back(p);
                    buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
                    st.push_back(state::ALIVE);
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
        if (st[0] != state::DEAD && fds[0].revents & ERR)
        {
            st[0] = state::DEAD;
        }
        if (st[0] == state::ALIVE && fds[0].revents & POLLIN)
        {
            int fd = accept(sfd, &addr, &addr_len);
            p.fd = fd;

            p.events = POLLIN | ERR;
            fds.push_back(p);
            st.push_back(state::ALIVE);
            buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
            readed.push_back(0);

            p.events = POLLOUT | ERR;
            fds.push_back(p);
            st.push_back(state::ALIVE);
            buffers.push_back(make_pair((char *)malloc(BUFFER_SIZE), 0));
            readed.push_back(0);

            printf("new client connected\n");
        }
        
        for (size_t i = 1; i < fds.size(); i++)
        {
            //if (i == 2)
            //{
            //    printf("i am 2\n");
            //}
            if (st[i] != state::DEAD && fds[i].revents & ERR)
            {
                st[i] = state::DEAD;
            }

            if (st[i] != state::DEAD && 
                buffers[i].second != BUFFER_SIZE && 
                fds[i].revents & POLLIN)
            {
                int r = read(fds[i].fd, buffers[i].first + buffers[i].second, BUFFER_SIZE - buffers[i].second);
                if (r >= 0)
                {
                    buffers[i].second += r;
                }
                else
                {
                    st[i] = state::DEAD;
                }
            }


            if (st[i] != state::DEAD &&
                buffers[i].second > 0 &&
                fds[i].revents & POLLOUT)
            {
                //printf("Я хочу писать!\n");
                print(fds[i].fd, &buffers[i]);
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
                    //printf("AAA!!!\n");
                    string key = next(command, ' ');
                    string value1 = next(command, ' ');
                    string value2 = next(command, '\n');
                    string id(argv[1]);
                    id.append(to_string(time(NULL)));
                    //printf("1\n");
                    auto it = m.find(key);
                    if (it == m.end())
                    {
                        if (value1 == COLLISION)
                        {
                            set<string> tmp_set = {id};
                            vector<string> tmp_v = {value1, value2};
                            m[key] = make_pair(tmp_set, tmp_v);
                        }
                    }
                    else
                    {
                        //printf("2\n");
                        string message;
                        if (it->second.first.find(id) == it->second.first.end())
                        {
                            it->second.first.insert(id);
                            //printf("3\n");
                            for (int j = it->second.second.size() - 1; j >= 0; j--)
                            {
                                if (it->second.second[j] == value1)
                                {
                                    if ((size_t)j != it->second.second.size() - 1)
                                    {
                                        it->second.second.push_back(COLLISION);
                                        message = "c ";
                                        message.append(id + " ");
                                        message.append(key + "\n");
                                    }
                                    break;
                                }
                            }
                            //printf("4\n");
                            if (message.size() == 0)
                            {
                                //it->second.first.insert(id);
                                it->second.second.push_back(value2);
                                //printf("%s\n", value2.c_str());
                                message = "a ";
                                message.append(id + " ");
                                message.append(key + " ");
                                message.append(value1 + " ");
                                message.append(value2 + "\n");
                            }
                            //printf("5\n");
                            for (size_t j = 3; j < fds.size(); j += 2)
                            {
                                add_message(buffers[j].first, &buffers[j].second, message);
                                
                            }
                        }
                    }
                }
                if (com == "p")
                {
                    //printf("PPP!!!\n");
                    string key = next(command, '\n');
                    auto it = m.find(key);
                    if (it != m.end())
                    {
                        string message = it->second.second[0];
                        for (size_t j = 1; j < it->second.second.size(); j++)
                        {
                            message.append("->" + it->second.second[j]);
                        }
                        message += "\n";
                        //printf("%s\n", message.c_str());
                        //printf("%lu\n", buffers[2].second);
                        add_message(buffers[2].first, &buffers[2].second, message);
                        //printf("%lu\n", buffers[2].second);
                    }

                }

            }

        }
        
        for (size_t j = 3; j < fds.size(); j += 2)
        {
        
            if (buffers[j].second != 0 && buffers[j].second > readed[j])
            {
                int res = next_token(buffers[j].first, readed[j], buffers[j].second, '\n');
                if (res == -1)
                {
                    readed[j] = buffers[j].second;
                }
                else
                {
                    string command(buffers[j].first, res);
                    string command2 = command;
                    buffers[j].second -= res;
                    readed[j] = 0;
                    memmove(buffers[j].first, buffers[j].first + res, buffers[j].second);

                    string com = next(command, ' ');
                    string id = next(command, ' ');
                    string key = next(command, ' ');
                    auto it = m.find(key);
                    if (com == "a")
                    {
                        //printf("AAA!!!\n");
                        string value1 = next(command, ' ');
                        string value2 = next(command, '\n');
                        //string id(argv[1]);
                        //id.append(to_string(time(NULL)));
                        //printf("1\n");
                        if (it == m.end())
                        {
                            if (value1 == COLLISION)
                            {
                                set<string> tmp_set = {id};
                                vector<string> tmp_v = {value1, value2};
                                m[key] = make_pair(tmp_set, tmp_v);
                            }
                        }
                        else
                        {
                            //printf("2\n");
                            string message;
                            if (it->second.first.find(id) == it->second.first.end())
                            {
                                it->second.first.insert(id);
                                //printf("3\n");
                                for (int j = it->second.second.size() - 1; j >= 0; j--)
                                {
                                    if (it->second.second[j] == value1)
                                    {
                                        if ((size_t)j != it->second.second.size() - 1)
                                        {
                                            it->second.second.push_back(COLLISION);
                                            message = "c ";
                                            id = string(argv[1]);
                                            id.append(to_string(time(NULL)));
                                            message.append(id + " ");
                                            message.append(key + "\n");
                                        }
                                        break;
                                    }
                                }
                                //printf("4\n");
                                if (message.size() == 0)
                                {
                                    //it->second.first.insert(id);
                                    it->second.second.push_back(value2);
                                    //printf("%s\n", value2.c_str());
                                    //message = "a ";
                                    //message.append(id + " ");
                                    //message.append(key + " ");
                                    //message.append(value1 + " ");
                                    //message.append(value2 + "\n");
                                    message = command2;
                                }
                                //printf("5\n");
                                for (size_t j = 3; j < fds.size(); j += 2)
                                {
                                    add_message(buffers[j].first, &buffers[j].second, message);
                                
                                }
                            }
                        }
                    }
                    if (com == "c")
                    {
                        if (it == m.end())
                        {
                            set<string> tmp_set = {id};
                            vector<string> tmp_v = {COLLISION, COLLISION};
                            m[key] = make_pair(tmp_set, tmp_v);
                        }
                        else
                        {
                            if (it->second.first.find(id) == it->second.first.end())
                            {
                                it->second.first.insert(id);
                                it->second.second.push_back(COLLISION);
                            }
                        }
                        for (size_t j = 3; j < fds.size(); j += 2)
                        {
                            add_message(buffers[j].first, &buffers[j].second, command2);
                        }
                    }
                }

            }

        }
    }
    
    close(sfd);
   
    return 0;
}
