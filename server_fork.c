/*********************************************************************************
 *      Copyright:  (C) 2022 Zhu Lijun
 *                  All rights reserved.
 *
 *       Filename:  server_fork.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/03/22)
 *         Author:  Zhu Lijun <3262465970@qq.com>
 *      ChangeLog:  1, Release initial version on "11/03/22 15:52:23"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#define BACKLOG 13

void child_process(int cli_fd);
void print_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("-p(--port):sepcify server port.\n");
	printf("-h(--Help):print this help information.\n");
	return ;
}

int main (int argc, char **argv)
{
	int listen_fd = -1;
    struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;
	socklen_t len;
	int port = 0;
	int rv = -1;
	int cli_fd;
	int on = 1;
	int ch;
	pid_t pid,pid1;

	struct option opts[] = {
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
	    {NULL, 0, NULL, 0}
	};

	while( (ch=getopt_long(argc, argv, "p:h", opts, NULL)) != -1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				print_usage(argv[0]);
			    return 0;
		}
	}
	
	if(!port)
	{
		printf("please input the port!\n");
		print_usage(argv[0]);
		return 0;
	}

	listen_fd = socket(AF_INET,SOCK_STREAM,0);
	if(listen_fd < 0)
	{
		printf("create socket failure: %s\n",strerror(errno));
		return -1;
	}
	
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	

	rv = bind(listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if(rv < 0)
	{
		printf("bind socket failure: %s\n",strerror(errno));
		return -2;
	}
	printf("socket[%d] bind on port[%d] successfully!\n",listen_fd,port);

	if(listen(listen_fd,BACKLOG) < 0)
	{
		printf("listen failure:%s\n",strerror(errno));
		return -3;
	}

	while(1)
	{
		cli_fd = accept(listen_fd,(struct sockaddr *)&cliaddr,&len);
		if(cli_fd <0 )
		{
			printf("accept client failure:%s\n",strerror(errno));
			return -4;
		}
		
		pid = fork();
		if(pid < 0 )
		{
			printf("fork() create child process failure:%s\n",strerror(errno));
			close(cli_fd);
			return -4;
		}
		else if(pid >0 )
		{
			close(cli_fd);
			continue;			
		}
		else if(0 == pid)
		{
			child_process(cli_fd);
			close(listen_fd);
			return 0;
		}
		pid1 = waitpid(pid,NULL,WNOHANG);
		if(pid1 == 0)
		{
			printf("the child process[%d] not No finish\n",getpid());
			sleep(1);
		}
		else if(pid1 == pid)
		{
			printf("successfully get child PID[%d]\n",getpid());
		}
		else
		{
			printf("get error\n");
		}
	}
	close(listen_fd);
	return 0;
}

void child_process(int cli_fd)
{
	char buf[1024];
	int	rv = -1;
	printf("child process PID[%d] start to commuicate whit client...\n",getpid());
	while(1)
	{
		rv = read(cli_fd,buf,sizeof(buf));
		if(rv < 0 )
		{   
			printf("read data from client failure:%s\n",strerror(errno));
			close(cli_fd);
			exit(0);
		}
		printf("The temperature('C) is : %s\n",buf);
		rv = write(cli_fd,buf,sizeof(buf));
		if(rv < 0 )
		{
			printf("write data  failure:%s\n",strerror(errno));
			close(cli_fd);
		}   
		close(cli_fd);
		exit(0);
	}         
}
