#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"work_pthread.h"


#define PORT 6000
#define IPSTR "127.0.0.1"
#define LIS_MAX 5

int create_socket();
int accept_fun(int sockfd);

int main()
{
    int sockfd = create_socket();
    if(sockfd == -1)
    {
        printf("create socket error\n");
        exit(0);
    }

    while(1)
    {
        int c = accept_fun(sockfd);
        if(c == -1)
        {
            printf("accept error\n");
            continue;
        }

        thread_start(c);
    }
}

int accept_fun(int sockfd)
{
    struct sockaddr_in caddr;
    int len = sizeof(caddr);
    int c = accept(sockfd,(struct sockaddr*)&caddr,&len);
    
    return c;
}
int create_socket()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1)
    {
        return -1;
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = inet_addr(IPSTR);

    int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res == -1)
    {
        perror("bind error\n");
        return -1;
    }

    listen(sockfd,LIS_MAX);

    return sockfd;

}
