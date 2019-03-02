#include"work_pthread.h"
#include<fcntl.h>

#define ARGC 10
/*启动线程*/

void ser_check(char *myargv[],char *check_buff)
{
    int pipefd[2];
    char *path[128] = {0};
    path[0] = "md5sum";
    path[1] = myargv[1];

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
    read(pipefd[0],check_buff,127);
}

void thread_start(int c)
{
    pthread_t id;
    pthread_create(&id,NULL,work_thread,(void*)c);
}

void get_argv(char buff[],char *myargv[])
{
    char *p = NULL;
    char *s = strtok_r(buff," ",&p);
    int i = 0;

    while(s != NULL)
    {
        myargv[i++] = s;
        s = strtok_r(NULL," ",&p);
    }
}

void send_file(int c,char *myargv[])
{
    if(myargv[1] == NULL)
    {
        send(c,"get:no file name!",17,0);
        return;
    }

    int fd = open(myargv[1],O_RDONLY);
    if(fd == -1)
    {
        send(c,"not found!",10,0);
        return;
    }

    int size = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    char cli_data[32] = {0};
    int size1 = 0;
    if(recv(c,cli_data,31,0) <= 0)
    {
        return;
    }

    if(strncmp(cli_data,"ok",2) != 0)
    {
        sscanf(cli_data,"%d",&size1);
        lseek(fd,size1,SEEK_SET);
        size = size - size1;
    }

    char check_buff[128] = {0};
    ser_check(myargv,check_buff);
    char status[32] = {0};
    sprintf(status,"ok#%d#",size);
    strcat(status,check_buff);
    send(c,status,strlen(status),0);
    char cli_status[32] = {0};

    char data[256] = {0};
    int num = 0;
    while((num=read(fd,data,256)) > 0)
    {
        send(c,data,num,0);
    }
    close(fd);
    return;
    
}

int recv_file(int c,char *myargv[])
{
    /*char tmp_name[10] = {0};
    strcpy(tmp_name,myargv[1]);
    strncat(tmp_name,"(tmp)",5);
*/
    int fd = open(myargv[1],O_WRONLY|O_CREAT,0600);
    if(fd == -1)
    {
        send(c,"err",3,0);
        return;
    }
    int cur_size = 0;
    if(access(myargv[1],F_OK) == 0)
    {
        char cli_size[256] = {0};
        cur_size = lseek(fd,0,SEEK_END);
        sprintf(cli_size,"%d",cur_size);
        send(c,cli_size,strlen(cli_size),0);
    }
    else
    {
        send(c,"ok",2,0);
    }

    char buff[128] = {0};

    if(recv(c,buff,127,0) <= 0)
    {
        return -1;
    }
    int size = 0;
    sscanf(buff+3,"%d",&size);

    int sizesum = size + cur_size;
    int num = 0;
    char data[256] = {0};
    while(1)
    {
        if(cur_size >= sizesum)
        {
           /* pid_t pid = fork();
            if(pid == 0)
            {
                char *path[128] = {0};
                path[0] = "mv";
                path[1] = tmp_name;
                path[2] = myargv[1];

                execvp(path[0],path);
                exit(0);
            }
            wait(NULL);
            */
            break;
        }
        num = recv(c,data,256,0);
        if(num <= 0)
        {
            close(fd);
            return 0;
        }
                       
        cur_size = cur_size + num;
        write(fd,data,num);

    }
    close(fd);
    char check_buff[128] = {0};
    ser_check(myargv,check_buff);
    send(c,check_buff,strlen(check_buff),0);
    return;
}

/*工作线程*/
void* work_thread(void* arg)
{
    int c = (int)arg;

    while(1)
    {
        char buff[128] = {0};
        int n = recv(c,buff,127,0);
        if(n <= 0)
        {
            close(c);
            printf("one client over\n");
            break;
        }

        printf("recv:%s\n",buff);
        
        char *myargv[ARGC] = {0};
        get_argv(buff,myargv);
        
        if(strcmp(myargv[0],"get") == 0)
        {
            send_file(c,myargv);
        }
        else if(strcmp(myargv[0],"put") == 0)
        {
            recv_file(c,myargv);
        }
        else
        {
            int fd[2];
            pipe(fd);
            pid_t pid = fork();
            if(pid == 0)
            {
                dup2(fd[1],2);
                dup2(fd[1],1);
                execvp(myargv[0],myargv);
                perror("exec error");
            
                exit(0);
            }
            close(fd[1]);
            wait(NULL);

            char read_buff[1024] = {0};
            //strcpy(read_buff,"ok#");
            read(fd[0],read_buff+strlen(read_buff),1000);
            if(strncmp(myargv[0],"ls",2) == 0)
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
            
            send(c,read_buff,strlen(read_buff),0);
        }
    }
}
