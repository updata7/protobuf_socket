#include "msg.pb.h"
#include "dealpack.h"
#include <iostream>
#include <string>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}while(0)

extern void do_client(int listenfd);	//deal with connect

int main(void)
{
	int count = 0;	
	while(1)
	{
		int listenfd = socket(PF_INET, SOCK_STREAM, 0);
		if(listenfd == -1)
		{
			sleep(4);	//make server enough time to deal with close
			ERR_EXIT("listenfd");
		}

		//server's address and port
		struct sockaddr_in servaddr;
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(5588);
		servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		//establish connect

		if(connect(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		{
			ERR_EXIT("connect");
		}

		struct sockaddr_in localaddr;
		socklen_t addrlen = sizeof(localaddr);
		if (getsockname(listenfd, (struct sockaddr*)&localaddr, &addrlen) < 0)
			ERR_EXIT("getsockname");
		printf("ip=%s port=%d\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));
		printf("count = %d\n", ++count);
	}

//	do_client(listenfd);
	return 0;
}
