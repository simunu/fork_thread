/*********************************************************************************
 *      Copyright:  (C) 2022 Zhu Lijun
 *                  All rights reserved.
 *
 *       Filename:  server_thread.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/04/22)
 *         Author:  Zhu Lijun <3262465970@qq.com>
 *      ChangeLog:  1, Release initial version on "11/04/22 06:40:19"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <ctype.h>

#define MSG_STR "Get the temperature successfully!"
#define BACKLOG 13
typedef void *(THREAD_BODY) (void *thread_arg);

void *thread_worker(void *ctx);
int thread_start(pthread_t * thread_id,THREAD_BODY * thread_workbody,void *thread_arg);

void *thread_worker(void *ctx)
{
	int		cli_fd;
	int		rv;
	char	buf[1024];
	int		i;
	if(!ctx)
	{
		printf("error: %s() get invalid input argments!\n",__FUNCTION__);
		pthread_exit(NULL);
	}
	cli_fd = (int)ctx;
	printf("thread worker[%d] start running...\n",pthread_self());
	while(1)
	{
		memset(buf,0,sizeof(buf));
		rv = read(cli_fd,buf,sizeof(buf));
		if(rv < 0 )
		{
			printf("read data from client failure:%s",strerror(errno));
			close(cli_fd);
			pthread_exit(NULL);
		}
		else if(rv==0)
		{
			printf("socket connect failure and thread will exit.\n");
			close(cli_fd);
			pthread_exit(NULL);
		}
		else if(rv > 0 )
		{
			printf("The temperature is :%s\n",buf);
		}
		rv = write(cli_fd,buf,rv);
		if(rv < 0 )
		{
			printf("write data failure:%s\n",strerror(errno));
			close(cli_fd);
			pthread_exit(NULL);
		}
	}
}

int thread_start(pthread_t * thread_id,THREAD_BODY *thread_workbody,void *thread_arg)
{ 
	pthread_attr_t  thread_attr;

	if(pthread_attr_init(&thread_attr))
	{   
		printf("pthread_attr_init() failure:%s\n",strerror(errno));
		return -1; 
	}   
	if(pthread_attr_setstacksize(&thread_attr,120*1024))
	{   
		printf("pthread_attr_setstacksize() failure:%s\n",strerror(errno));
		return -2; 
	}   
	if(pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED))
	{   
		printf("pthread_attr_setdetachstate() failure:%s\n",strerror(errno));
		return -3; 
	}   

	if(pthread_create(thread_id,&thread_attr,thread_workbody,thread_arg))
	{   
		printf("create thread failure:%s\n",strerror(errno));
		return -4; 
	}  
	return 0;
}



void print_usage(char *progname)
{
	printf("%s usage: \n", progname);
	printf("-p(--port): sepcify server listen port.\n");
	printf("-h(--Help): print this help information.\n");
	return ;
}

int main(int argc,char **argv)
{
	int listen_fd = -1;
	int rv = -1;
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;
	socklen_t len;
	int port = 0;
	int cli_fd;
	int ch;
	int on = 1;
	pthread_t tid;

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

	if( !port )
	{
		print_usage(argv[0]);
		return 0;
	}
	listen_fd=socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		printf("Create socket failure: %s\n", strerror(errno));
		return -1;
	}
	printf("Create socket[%d] successfully!\n", listen_fd);
	
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	rv=bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(rv < 0)
	{
		printf("Socket[%d] bind failure: %s\n", listen_fd, strerror(errno));
	    return -2;
	}

	listen(listen_fd,BACKLOG);
	while(1)
	{
		cli_fd=accept(listen_fd, (struct sockaddr *)&cliaddr, &len);
		if(cli_fd < 0)
		{
			printf("accept client failure: %s\n", strerror(errno));
			continue;
		}
		printf("accept new client successfully\n");
		thread_start(&tid,thread_worker,(void *)cli_fd);
	}
	close(listen_fd);
	return 0;
}



