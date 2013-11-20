#include "dealpack.h"

#include <iostream>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}while(0)


extern void do_service(int listenfd);

int main(void)
{
	//listening for client
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if(listenfd == -1)
	{
		ERR_EXIT("socket");
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr)); //init address
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5588); //open a port for client
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // open the address for client

	//for bind the address which is being used.
	int on = 1;	
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
	{
		ERR_EXIT("setsockopt");
	}

	//bind the port and address
	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	{
		ERR_EXIT("bind");
	}

	//listening for client
	if(listen(listenfd, SOMAXCONN) == -1)
	{
		ERR_EXIT("listen");
	}

	do_service(listenfd);
	return 0;
}
