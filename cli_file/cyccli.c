#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<termios.h>

//#define UP "^[[A"
//#define DOWN "^[[B"

void ls_adjust(char *read_buff)
{
    struct winsize size;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
    char tmp[128] = {0};
    strcpy(tmp,buff);

    char* myargv[10] = {0};
    char* s = strtok(tmp," ");
    int len = strlen(s);
    int i = 0;
    while(s != NULL)
    {
        myargv[i++] = s;
        s = strtok(NULL," ");
        if(strlen(s) > len)
        {
            len = strlen(s);
        }
    }
    int num = size.ws_row / (len+1);
    i = 0;
    while(read_buff == '\0')
    {
        printf("");
    }
    
}

void cli_check(char *check_buff,char *name)
{
    int pipefd[2];
    char *path[30] = {0};
    path[0] = "md5sum";
    path[1] = name;

    pipe(pipefd);
    pid_t pid = fork();
    if(pid == 0)
    {
        dup2(pipefd[1],1);
        execvp(path[0],path);
        exit(0);
    }
    close(pipefd[1]);
    wait(NULL);
    char read_buff[128] = {0};
    read(pipefd[0],read_buff,127);
    printf("ser_md5:%s",check_buff);
    printf("cli_md5:%s",read_buff);
    if(strcmp(check_buff,read_buff) == 0)
    {
        printf("check:right\n");
    }
    else
    {
        printf("check:error\n");
    }
}


int recv_file(int sockfd,char* name)
{

    int fd = open(name,O_WRONLY|O_CREAT,0600);
    if(fd == -1)
    {
        send(sockfd,"err",3,0);
        return;
    }

    int cur_size = 0;
    if(access(name,F_OK) == 0)
    {
        char ser_size[256] = {0};
        cur_size = lseek(fd,0,SEEK_END);
        sprintf(ser_size,"%d",cur_size);
        send(sockfd,ser_size,strlen(ser_size),0);
    }
    else
    {
        send(sockfd,"ok",2,0);
    }
    int size = 0;
    char buff[128] = {0};
    if(recv(sockfd,buff,127,0) <= 0)
    {
        return -1;
    }
    if(strncmp(buff,"ok",2) != 0)
    {
        printf("%s\n",buff);
        return 0;
    }
    char check_buff[128] = {0};
    sscanf(buff+3,"%d",&size);
    char* s = strtok(buff,"#");
    s = strtok(NULL,"#");
    s = strtok(NULL,"#");
    strcat(check_buff,s);
    printf("file(%s):%d\n",name,size);

    int sizesum = size+cur_size;
    int num = 0;
    char data[256] = {0};
    while(1)
    {
        if(cur_size >= sizesum)
        {
            break;
        }
        num = recv(sockfd,data,256,0);
        if(num <= 0)
        {
            close(fd);
            return 0;
        }
        cur_size = cur_size + num;

        write(fd,data,num);
        float f = cur_size * 100.0 / sizesum;
        printf("download:%.2f%%\r",f);
        fflush(stdout);
    }
    printf("\n");
    close(fd);

    cli_check(check_buff,name);
}

int send_file(int sockfd,char *name)
{
    char buff[128] = {0};
    if(name == NULL)
    {
        printf("put:no file name!");
        return;
    }

    int fd = open(name,O_RDONLY);
    if(fd == -1)
    {
        printf("not found!\n");
        return;
    }

    int sumsize = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    int size = 0;
    int size1 = 0;
    char ser_data[32] = {0};
    if(recv(sockfd,ser_data,31,0) <= 0)
    {
        return;
    }
                 
    if(strncmp(ser_data,"ok",2) != 0)
    {
        sscanf(ser_data,"%d",&size1);
        lseek(fd,size1,SEEK_SET);
        size = sumsize - size1;
    }
                                     
    char status[32] = {0};

    printf("file(%s):%d\n",name,size);
    sprintf(status,"ok#%d",size);
    send(sockfd,status,strlen(status),0);
                          
    char data[256] = {0};
    int num = 0;
    int cur_size = size1;
    while((num=read(fd,data,256)) > 0)
    {
        send(sockfd,data,num,0);
        
        cur_size = cur_size + num;
        float f = cur_size * 100.0 / sumsize;
        printf("upload:%.2f%%\r",f);
        fflush(stdout);
    }
    printf("\n");
                              
    close(fd);
    
    char check_buff[128] = {0};
    recv(sockfd,check_buff,127,0);
    cli_check(check_buff,name);
                                
    return; 

}
int main()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd != -1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res != -1);

    while(1)
    {
        char buff[128] = {0};

        printf("Connect success ~]$ ");
        fflush(stdout);
        fgets(buff,128,stdin);
        buff[strlen(buff)-1] = 0;


        if(strncmp(buff,"end",3) == 0)
        {
            break;
        }
        if(buff[0] == 0)
        {
            continue;
        }
        

        char tmp[128] = {0};
        strcpy(tmp,buff);

        char* myargv[10] = {0};
        char* s = strtok(tmp," ");

        int i = 0;
        while(s != NULL)
        {
            myargv[i++] = s;
            s = strtok(NULL," ");
        }

        if(strncmp(myargv[0],"get",3) == 0)
        {
            send(sockfd,buff,strlen(buff),0);
            recv_file(sockfd,myargv[1]);
        }
        else if(strncmp(myargv[0],"put",3) == 0)
        {
            send(sockfd,buff,strlen(buff),0);
            send_file(sockfd,myargv[1]);
        }
        else
        {
            int pfd[2];
            pipe(pfd);
            if(buff[0] == '*')
            {
                char *argv[10] = {0};
                sscanf(buff+1,"%s",&buff);
                char* s = strtok(buff," ");

                int i = 0;
                while(s != NULL)
                {
                    argv[i++] = s;
                    s = strtok(NULL," ");
                }
                pid_t pid = fork();
                if(pid == 0)
                {
                    dup2(pfd[1],1);
                    execvp(argv[0],argv);
                }
            }
            else
            {
                send(sockfd,buff,strlen(buff),0);

                char read_buff[1024] = {0};
                recv(sockfd,read_buff,1023,0);
                printf("%s",read_buff);
                continue;
            }
            close(pfd[1]);
            char read_buff[128] = {0};
            read(pfd[0],read_buff,127);
            if(strncmp(myargv[0],"*ls",3) == 0)
            {
                int i = 0;
                while(i < strlen(read_buff)-1)
                {
                    if(read_buff[i] =='\n')
                    {
                        read_buff[i] = '\t';
                    }
                i++;
                }
            }

            printf("%s",read_buff);
        }
    }

    close(sockfd);

}
