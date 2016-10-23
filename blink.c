//#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#define THREAD_NO 2
struct arg_struct{
    int id;
    int time;
    int port;
};
struct port_thread{
    pthread_t thread;
    int listenfd;
};

struct port_thread threads[THREAD_NO];
struct arg_struct args_array[THREAD_NO];

void sig_alrm(int signo){
    int i = 0;
    for(i=0; i<THREAD_NO; i++){
        close(threads[i].listenfd);
        pthread_kill(threads[i].thread, SIGUSR1);
    }
}
void handler(int signum)
{
    pthread_exit(NULL);
}
void *set(void *arguments){

    struct arg_struct *args = arguments;
    int connfd = 0;
    threads[args->id].listenfd = 0;
    struct sockaddr_in serv_addr; 

    char sendBuff[1025];
    time_t ticks; 

    threads[args->id].listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(args->port); 

    bind(threads[args->id].listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(threads[args->id].listenfd, 10);
    while(1)
    {
        connfd = accept(threads[args->id].listenfd, (struct sockaddr*)NULL, NULL); 
        close(connfd);
        sleep(1);
    } 
    //pause();
    //printf("Closing port\n");
    //close(listenfd);
    //pause();
}

int main(int argc, char *argv[])
{
    signal(SIGUSR1, handler);
    struct sigaction act;
    act.sa_handler = sig_alrm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, 0);
    

    
    alarm(10);
    int i;
    for(i = 0; i < THREAD_NO; i++){
        //struct arg_struct args;
        args_array[i].id = i;
        args_array[i].time = 10;
        args_array[i].port = 5000 + i;
        printf("[%d]\n", args_array[i].port);
        pthread_create(&(threads[i].thread), NULL, &set, (void *)&args_array[i]);
    }
    for(i = 0; i < THREAD_NO; i++){
        pthread_join(threads[i].thread, NULL);
    }
    return 0;
}
