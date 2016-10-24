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
#include <time.h>
#include <stdlib.h>

#define THREAD_NO 16
#define WAIT_TIME 2
struct arg_struct{
    int id;
    int time;
    int port;
};
struct port_thread{
    pthread_t thread;
    int listenfd;
    int active;
};

struct port_thread threads[THREAD_NO];
struct arg_struct args_array[THREAD_NO];

void sig_alrm(int signo){
    int i = 0;
    for(i=0; i<THREAD_NO; i++){
        if(threads[i].active == 1){
            close(threads[i].listenfd);
            printf("Waiting from thread\n");
            pthread_kill(threads[i].thread, SIGUSR1);
        }
    }
    printf("Done killing threads\n");
}
void handler(int signum)
{
    printf("I'm exiting\n");
    pthread_exit(NULL);
}
void *set(void *arguments){

    struct arg_struct *args = arguments;
    printf("Setting up[%d]:[%d]\n", args->id, args->port);
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

void blinkBuffer(char buf[THREAD_NO]){
    alarm(WAIT_TIME);
    
    int i;
    int activeThreads=0;
    for(i = 0; i < THREAD_NO; i++){
        if(buf[i]=='1'){
            activeThreads++;
        }
    }
    for(i = 0; i < THREAD_NO; i++){
        if(buf[i]=='1'){
            //struct arg_struct args;
            args_array[i].id = i;
            //args_array[i].time = 10;
            args_array[i].port = 5000 + i;
            threads[i].active = 1;
            printf("[%d]\n", args_array[i].port);
            pthread_create(&(threads[i].thread), NULL, &set, (void *)&args_array[i]);
        }
        else{
            threads[i].active = 0;
        }
    }
    sleep(1);
    printf("Waiting for join\n");
    for(i = 0; i < THREAD_NO; i++){
        if(threads[i].active == 1){
            pthread_join(threads[i].thread, NULL);
        }
    }
    printf("Joined!\n");
}
void sendStr(char sampleStr[1024]){
    signal(SIGUSR1, handler);
    struct sigaction act;
    act.sa_handler = sig_alrm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, 0);
    int k, counter;
    char buf[THREAD_NO];
    for(counter = 0; counter < 1024; ){
        if(counter%THREAD_NO==0 && counter !=0){
            blinkBuffer(buf);
        }
        for(k = 0; k < THREAD_NO; k++){
            buf[counter%THREAD_NO] = sampleStr[counter];
            counter++;
        }
    }
}
int main(int argc, char *argv[])
{
    srand(time(NULL));
    char sampleStr[1024];
    int i;
    //Fancy string simulator
    for(i=0; i< 1024; i++){
        int r = rand()%2; 
        if(r%2==0){
            sampleStr[i] = '0';
        }else{
            sampleStr[i] = '1';
        }
    }
    sendStr(sampleStr);
    
    
    return 0;
}
