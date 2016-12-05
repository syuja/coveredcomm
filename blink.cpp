/********
* compile with -pthread
* g++ -pthread -std=c++11 blink.cpp -o blink
********/


#include <iostream>
#include <vector>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>
#include <strings.h>
#include <time.h>
#include <stdlib.h>
#include <chrono>
#include <pthread.h>

  using namespace std;

#define THREAD_NO 8 //also the number of ports
#define WAIT_TIME 2
#define TIME_OUT 10
//fgets stops at MAX_BUFFER_SIZE - 1, and appends \n at MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 8
#define MINPORT 1025
#define MAXPORT 65535
#define LISTENQ 10 //for listen

static int START_PORT = -1;
bool allconnections[THREAD_NO];

vector<int> all_sockfds;

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

//SIGNAL HANDLERS
//main thread handler, receives alarm signals threads
void sig_alrm(int signo){
    int i = 0;
    for(i=0; i<THREAD_NO; i++){
        if(threads[i].active == 1){
            close(threads[i].listenfd);
            printf("Waiting from thread\n");
            //main thread signals each individual thread
            pthread_kill(threads[i].thread, SIGUSR1);
        }
    }
    printf("Done killing threads\n");
}

//individual thread handlers, exits threads
void handler(int signum){
    printf("I'm exiting\n");
    pthread_exit(NULL);
}

//thread being killed after time out!
void thread_handler(int signo){
  //exit, so mainthread can do pthread_join
   pthread_exit(NULL);
}

//FUNCTION DECLARATIONS:
//find starting port number
int findStartPort();

//indicate start location
void startConnection();

void *startAccept(void *arg);

bool allConnected();


//separates bits: 1 bit to each thread
void sendStr(char sampleStr[MAX_BUFFER_SIZE]);

void blinkBuffer(char buf[THREAD_NO]);

void *set(void *arguments);



int main(int argc, char *argv[]){

    START_PORT = findStartPort();

    //blinks until client connects
    startConnection();

    // char sampleStr[MAX_BUFFER_SIZE];
    // //fgets appends newline at MAX_BUFFER_SIZE
    // while(scanf("%s", sampleStr)){
    //   //sendStr ==> creates threads to send key char in binary
    //   sendStr(sampleStr);
    // }

    //close the connection

    return 0;
}

//flashes until the client sees it
void startConnection(){
  struct sockaddr_in sock_addr;
  pthread_t threads[THREAD_NO];
int j = 0;
  do{
    cerr << "Loop = " << j << endl;
    for(int i = 0; i < THREAD_NO-1; ++i){
      allconnections[i] = false;
    }

    for(int i = 0; i < THREAD_NO; ++i){
      all_sockfds.push_back(socket(AF_INET, SOCK_STREAM, 0));

      memset(&sock_addr, 0, sizeof(sock_addr));
      sock_addr.sin_family = AF_INET;
      sock_addr.sin_port = htons(START_PORT+i);
      sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

      //CREATE sockets and push them
      ::bind(all_sockfds.back(), (struct sockaddr*) &sock_addr, sizeof(sock_addr));

      listen(all_sockfds.back(),LISTENQ);
      cerr << "call pthread_create" << endl;
      pthread_create(&(threads[i]), NULL, &startAccept, (void*) &i);
    }
    //
    cerr << "main thread time_out sleeping" << endl;
    sleep(TIME_OUT);
    cerr << "main thread time_out sleeping done" << endl;
    //kill all of them
    for(int i = 0; i < THREAD_NO; ++i){
      cerr << "killing thread == " << threads[i] << endl;
      pthread_kill(threads[i],SIGUSR2);
    }

    //join
    for(int i = 0; i < THREAD_NO; ++i){
      cerr << "joining thread == " << threads[i] << endl;
      pthread_join(threads[i], NULL);
    }
    //close all of the sockets
    for(int i = 0; i < THREAD_NO; ++i){
      cerr << "closing socket = " << all_sockfds[i] << endl;
      close(all_sockfds[i]);
    }
    ++j;
  }while(!allConnected());
  cerr << "ALL CONNECTED!!" << endl;

}

//establish connection with client
void *startAccept(void *arg){
  struct sockaddr_in recaddr;
  socklen_t len = sizeof(recaddr);
  memset(&recaddr, 0, sizeof(recaddr));
  recaddr.sin_family = AF_INET;

  //register signal handler
  struct sigaction thread_sig;
  thread_sig.sa_handler = thread_handler;
  sigemptyset(&thread_sig.sa_mask);
  thread_sig.sa_flags = 0;
  sigaction(SIGUSR2, &thread_sig, 0);

  //accept
  int i = *(int *) arg;
  cerr << "i = " << i << endl;

  int maxfd;

  //one thread ==> clock
  if(i == THREAD_NO - 1){
    //blink
    while(!allConnected()){
      listen(all_sockfds[i], LISTENQ);
      sleep(WAIT_TIME);
      close(all_sockfds[i]);
      sleep(WAIT_TIME);
    }
  }
  //time
  else{
    accept(all_sockfds[i], (struct sockaddr*) &recaddr, &len);
    allconnections[i] = true;
  }
  pthread_exit(NULL);
}


bool allConnected(){
  bool isall = true;
  for(int i = 0; i < THREAD_NO-1 && isall; ++i){
    isall = isall && allconnections[i];
    cerr << "allconnections[" << i << "] = " << allconnections[i] << endl;
  }
  return isall;
}

//finds the port to start from; sets global START_PORT
int findStartPort(){
  vector<int> connected;
  char hostname[] = "hostname";
  int reuseaddr = 1;

  for(int portno = MINPORT; portno < MAXPORT - THREAD_NO; portno++){
    //THREAD_NO consecutive
    for(int j = 0; j < THREAD_NO; ++j){
      int sockfd;
      struct sockaddr_in serv_addr;
      //struct hostent *server;

      //socket for finding available port
      if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("findStartPort socket error: ");
        continue;
      }

      //allow reusing that socket again quickly
      if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (char *) &reuseaddr, sizeof(reuseaddr))) < 0){
          perror("findStartPort setsockopt error: ");
      }

      //get my ip and port
      memset((void *) &serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(portno+j);
      serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

      //try binding
      if(::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        //don't print
        perror("findStartPort bind error: ");
      }
      //bind was successful
      else{
        connected.push_back(portno+j);
      }
      //close it
      close(sockfd);
    }//end inner for loop

    if(connected.size() == THREAD_NO){
      //found THREAD_NO consecutive ports
      return connected[0];
    }
    else{
      //check the next 8
      connected.clear();
    }

  }//end outer for loop
  return -1;
}

//separates bits for each thread
void sendStr(char sampleStr[MAX_BUFFER_SIZE]){
  //prepare signal handlers
  struct sigaction act, act2;

  sigemptyset(&act.sa_mask);
  sigemptyset(&act2.sa_mask);
  act.sa_handler = sig_alrm;
  act.sa_flags = 0;
  act2.sa_handler = handler;
  act2.sa_flags = 0;

  sigaction(SIGALRM, &act, 0);
  sigaction(SIGUSR1, &act2, 0);


  int k, counter;
  char buf[THREAD_NO];

  for(counter = 0; counter < MAX_BUFFER_SIZE; ){
      if(counter%THREAD_NO==0 && counter !=0){
          blinkBuffer(buf);
      }
      for(k = 0; k < THREAD_NO; k++){
          buf[counter%THREAD_NO] = sampleStr[counter];
          counter++;
      }
    }
}

//call pthread_create
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
            args_array[i].port = START_PORT + i;
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

void *set(void *arguments){

    struct arg_struct *args = (arg_struct *) arguments;
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

    bind(threads[args->id].listenfd, (struct sockaddr*)&serv_addr,
      sizeof(serv_addr));

    listen(threads[args->id].listenfd, 10);
    while(1)
    {
        connfd = accept(threads[args->id].listenfd,
            (struct sockaddr*)NULL, NULL);
        close(connfd);
        sleep(1);
    }
}
