#include "dealpack.h"
#include "msg.pb.h"

#include <iostream>
#include <vector>
#include <algorithm>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}while(0)

typedef std::vector<struct epoll_event> EventList;

static void activate_nonblock(int conn);

void do_service(int listenfd)
{
	Data::datatype msg;
	//for epoll
        std::vector<int> clients;
        int epollfd;
        epollfd = epoll_create1(EPOLL_CLOEXEC);

        struct epoll_event event;
        event.data.fd = listenfd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);

        EventList events(16);   //init 16 events

        //client's address
        struct sockaddr_in peeraddr;
        socklen_t len = sizeof(peeraddr);

        //accept connect from client
        int conn;
        int i;

	char recvbuf[1024] = {0};
	char sendbuf = '0';

        int nready;
	int ret;
        while(1)
        {
                nready = epoll_wait(epollfd, &*events.begin(), static_cast<int>(events.size()), -1);
                if(nready == -1)
                {
                        if(errno == EINTR)
                                continue;

			ERR_EXIT("epoll_wait");
		}
		if(nready == 0)
			continue;
		
		if((size_t)nready == events.size())
                        events.resize(events.size()*2);

                for(i=0; i<nready; i++)
                {
			//client want to connect
                        if(events[i].data.fd == listenfd)
                        {
                                if((conn=accept(listenfd, (struct sockaddr*)&peeraddr, &len)) == -1)
                                {
                                ERR_EXIT("accept");
                                }
				std::cout<<"client: ip is "<<inet_ntoa(peeraddr.sin_addr)<<" port is "<<ntohs(peeraddr.sin_port)<<" connected."<<std::endl;
                                clients.push_back(conn);

                                activate_nonblock(conn);        //set nonblock
                                event.data.fd = conn;
                                event.events = EPOLLIN | EPOLLET;
                                epoll_ctl(epollfd, EPOLL_CTL_ADD, conn, &event);
                               
                        }
			//something can be read
			else if(events[i].events & EPOLLIN)
			{
				conn = events[i].data.fd;
				if(conn < 0)
					continue;
				memset(recvbuf, 0, sizeof(recvbuf));
                		ret = recv(conn, recvbuf, sizeof(recvbuf), 0);
				if(ret == -1)
					ERR_EXIT("recv");

                		if(ret == 0)
                		{
					struct sockaddr_in clientaddr;
					socklen_t client_len = sizeof(clientaddr);
					if(getpeername(conn, (struct sockaddr*)&clientaddr, &client_len) == -1)
						ERR_EXIT("getpeername");
                        		std::cout<<"client ip is "<<inet_ntoa(clientaddr.sin_addr)<<" port is "<<ntohs(clientaddr.sin_port)<<" close."<<std::endl;
                        		close(conn);

					event = events[i];
					epoll_ctl(epollfd, EPOLL_CTL_DEL, conn, &event);
					clients.erase(std::remove(clients.begin(), clients.end(), conn), clients.end());
					continue;
                		}

				//decode data
				std::string data = recvbuf;
                                msg.ParseFromString(data);
                                std::cout<<"From "<<msg.address()<<" "<<msg.port()<<": ";
                                std::cout<<msg.str()<<std::endl;
				if(send(conn, &sendbuf, strlen(&sendbuf), 0) == -1)
				ERR_EXIT("writen");
			}
                }
        }

	close(conn);
}

//set nonblock
static void activate_nonblock(int conn)
{
	int ret;
	int flags = fcntl(conn, F_GETFL);
	if(flags == -1)
	{
		ERR_EXIT("fcntl");
	}

	flags |= O_NONBLOCK;
	ret = fcntl(conn, F_SETFL, flags);
	if(ret == -1)
		ERR_EXIT("fcntl");
}

// get local ip
static int getlocalip(char *ip)
{
        char host[100] = {0};
        if(gethostname(host, sizeof(host)) < 0)
        {
                ERR_EXIT("gethostname");
        }
        struct hostent *hp;
        if((hp = gethostbyname(host)) == NULL)
        {
                ERR_EXIT("gethostbyname");
        }
        strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
        return 0;
}

//get local port
static unsigned short getlocalport(int listenfd)
{
	unsigned short port;
        struct sockaddr_in localaddr;
        socklen_t addrlen = sizeof(localaddr);
        if(getsockname(listenfd, (struct sockaddr *)&localaddr, &addrlen) < 0)
        {
                ERR_EXIT("getsockname");
        }

        port = ntohs(localaddr.sin_port);
        return port;
}

void do_client(int listenfd)
{
	Data::datatype msg;
	std::string str;
        std::string data;
	char sendbuf[1024] = {0};
	char recvbuf = '0';
        char ip[100];
        unsigned short port;
        getlocalip(ip);         //get local ip address
        port = getlocalport(listenfd);	//get local port
	unsigned short port1 = port;	//must be have,for port,or else port would change,don't know why

	//select
	fd_set rset;
	FD_ZERO(&rset);

	int nready;
	int maxfd;
	int fd_stdin = fileno(stdin);
	if(fd_stdin > listenfd)
		maxfd = fd_stdin;
	else
		maxfd = listenfd;

	int stdineof = 0;

	int flag = 1;
        while(1)
        {
		if(flag)
		{
          		std::cout<<"input the message you want to send: ";
               		fflush(stdout);
			flag=0;
		}
		if(stdineof == 0)
			FD_SET(fd_stdin, &rset);
		FD_SET(listenfd, &rset);
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if(nready == -1)
			ERR_EXIT("select");

		if(nready == 0)
			continue;
		if(FD_ISSET(fd_stdin, &rset))
                {
			getline(std::cin, str);
                        memset(sendbuf, 0, sizeof(sendbuf));
	
			//encode data
                        msg.set_port(port1);
                        msg.set_str(str);
			msg.set_address(ip);
                        msg.SerializeToString(&data);
                        strcpy(sendbuf, data.c_str());
                        if(send(listenfd, sendbuf, strlen(sendbuf), 0) == -1)
                        {
                                ERR_EXIT("send");
                        }
                std::cout<<"input the message you want to send: ";
                fflush(stdout);
                }
		
		if(FD_ISSET(listenfd, &rset))
		{
			
			int ret = recv(listenfd, &recvbuf, strlen(&recvbuf), 0);
			if(ret == -1)
				ERR_EXIT("read");

			else if(ret == 0)
			{
				std::cout<<std::endl;
				std::cout<<"reminder:server closed."<<std::endl;
				break;
			}	
			
		}
	
        }

	close(listenfd);
}
