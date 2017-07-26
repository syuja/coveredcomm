/************
  Compile :  make blink
  Run: ./blink [num_ports] [wait_time]

  Requires output from ./key

  It finds NUM_PORTS contiguous ports. Listens for client. Begins transmission.
************/

#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <bitset>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

  using namespace std;


/********************
*GLOBAL VARIABLES:
********************/
#define LISTENQUEUE 10

int NUM_PORTS = 8;                  //default
int WAIT_TIME = 2;
vector <string> BINARYKEY;          //holds final key in binary multiples
int START_PORT = 1025;

vector<int> SOCKETFDS;              //contains socket file descriptors
vector<bool> CONNECTION_STATE;      //state of each port


/********************
*FUNCTIONS:
********************/
//SETTING UP
void parseArgs(int arg_count, char** argv);
void readKey();
int findStartPort();

//INITIATE CONNECTION
//Check for Listener/Client
void startConnection();
void registerSignalHandler(string signal);
bool allConnected();
void createSockets(const string sock_map);
void bindAndListen(const string sock_map);
void terminateThreads(vector<pthread_t> tid, const string sock_map, string sig);
void *threadAccept(void *arg);     //thread starter function

//START TRANSMISSION
void sendMessage();
void *threadSend(void *arg);      //thread starter function

//SIGNAL HANDLERS
void threadSignalHandler(int signo){
  //exit, so thread can join
  pthread_exit(NULL);
}

//UTILITY
template<typename T>
  void printVector(vector<T> vec);

template<typename T>
  void printVecVector(vector<T> vec);


int main(int argc, char** argv){
  //SET UP

  cout << "\nServer starting..." << endl;
  //parse arguments
  parseArgs(argc, argv);
  //read binary key
  readKey();
  //find starting port
  cout << "Finding Start Port for Transmission... \n";
  START_PORT = findStartPort();
  cout << "Starting Port : " << START_PORT << endl;

  //START ESTABLISHING CONNECTIONS
  cout << "Awaiting Client: \n";
  startConnection();

  cout << "\nClient Connected... \nBeginning Transmission..." << endl;

  //START TRANSMISSION
  sendMessage();

  cout << "\nFinished Transmission...\nexiting...\n";
  return 0;
}



/********************
*SETTING UP to TRANSMIT
********************/
//read num_ports
void parseArgs(int arg_count, char** argv){
  //need at least 3 arguments: NUM_PORTS and 1 path
  if(arg_count < 3){
    cerr << "Error blink.cpp: parseArgs not enough Arguments" << endl;
    exit(1);
  }
  //saves the number of ports
  NUM_PORTS = stoi(argv[1]);
  WAIT_TIME = stoi(argv[2]);

  cout << "NUM_PORTS = " << NUM_PORTS <<
  " WAIT_TIME = " << WAIT_TIME << endl;

  //initialize global variables
  SOCKETFDS = vector<int> (NUM_PORTS);
  CONNECTION_STATE = vector<bool> (NUM_PORTS);
}

//read keyconv.cpp output
void readKey(){
  //reading the binary key
  string line;
  while(!cin.eof()){
    cin >> line;
    if(line != "")
      BINARYKEY.push_back(line);
    line.clear();
  }
}

//finds NUM_PORTS consecutive ports and returns first port number
int findStartPort(){
  //structure to check for NUM_PORTS continuous ports
  vector<int> connected;

  for(int portnum = START_PORT; portnum < 65535 - NUM_PORTS; ++portnum){
    for(int i = 0; i < NUM_PORTS; ++i){
        //create sockets for each port
        int sockfd;
        struct sockaddr_in serv_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        //get ip and port
        memset((void *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portnum+i);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        //try binding
        if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
          //don't print
          perror("Error: blink.cpp findStartPort port is in use...\n\
          skipping... ");
          //close just in case
          close(sockfd);

          //skip a few
          portnum = portnum + NUM_PORTS;
          connected.clear();
          continue;
        }
        //bind was successful
        else{
          connected.push_back(portnum+i);
        }
        //close it
        close(sockfd);
      }

      //check that we found NUM_PORTS contiguous, available ports
      if(connected.size() == NUM_PORTS){
        //return starting port
        return connected[0];
      }
      else{
        //start search all over again
        connected.clear();
      }
    }

  //failure to find NUM_PORTS available ports
  cerr << "Error blink.cpp: findStartPort not enough \n\
    available contiguous ports" << endl;
  exit(1);
}

/********************
*INITIATE CONNECTION
********************/
void startConnection(){
  vector<pthread_t> tid(NUM_PORTS);
  vector<int> thread_id(NUM_PORTS);
  string sock_map(NUM_PORTS, '1');

  for(int i = 0; i < thread_id.size(); ++i){
    thread_id[i] = i;
  }

  //register a signal handler for SIGUSR2
  registerSignalHandler("SIGUSR2");

  do{
    //reset vectors
    CONNECTION_STATE.clear();
    SOCKETFDS.clear();

    CONNECTION_STATE.resize(NUM_PORTS, false);
    SOCKETFDS.resize(NUM_PORTS, -1);

    //create sockets for each port
    createSockets(sock_map);

    //bind and listen on them
    bindAndListen(sock_map);

    //wait for connection (each thread waits)
    for(int i = 0; i < NUM_PORTS; ++i){
      //pthread_create();
      pthread_create((pthread_t *) &(tid[i]), NULL, &threadAccept,
                      (void *) &thread_id[i]);
    }

    //main thread wait a few seconds!!
    usleep(WAIT_TIME);

    //kill, join threads and close sockets
    terminateThreads(tid, sock_map, "SIGUSR2");

    cout << "Connection State: ";
    printVector(CONNECTION_STATE);
  } while(!allConnected());
}

//signal handler for pthreads
void registerSignalHandler(string signal){
  //signal handler for SIGUSR2
  struct sigaction sig_struct;
  sig_struct.sa_handler = threadSignalHandler;
  //clear the signal set
  sigemptyset(&sig_struct.sa_mask);
  sig_struct.sa_flags = 0;

  if(signal == "SIGUSR2")
    sigaction(SIGUSR2, &sig_struct, 0);
  else
    sigaction(SIGUSR1, &sig_struct, 0);

}

//create the sockets and set reuse option
void createSockets(const string sock_map){
  int reuseaddr = 1;

  for(int i = 0; i < sock_map.size(); ++i){
    //sock_map may be smaller than NUM_PORTS
    if(sock_map[i] == '1'){
      SOCKETFDS[i] = socket(AF_INET, SOCK_STREAM, 0);

      //allow reuse
      setsockopt(SOCKETFDS[i], SOL_SOCKET, SO_REUSEPORT,
        (char *) &reuseaddr, sizeof(reuseaddr));
      setsockopt(SOCKETFDS[i], SOL_SOCKET, SO_REUSEADDR,
        (char *) &reuseaddr, sizeof(reuseaddr));

    }
  }
}

//bind and listen on all sockets
void bindAndListen(const string sock_map){
  struct sockaddr_in sock_addr;

  for(int i = 0; i < sock_map.size(); ++i){
    if(sock_map[i] == '1'){
      memset(&sock_addr, 0, sizeof(sock_addr));
      sock_addr.sin_family = AF_INET;
      sock_addr.sin_port = htons(START_PORT+i);
      sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

      //CREATE sockets and push them
      bind(SOCKETFDS[i], (struct sockaddr*) &sock_addr, sizeof(sock_addr));

      listen(SOCKETFDS[i],LISTENQUEUE);
    }
  }
}

void *threadAccept(void *arg){
  //cast
  int thread_ind = *(int *) arg;

  //struct for accepting
  struct sockaddr_in recaddr;
  socklen_t len = sizeof(recaddr);
  memset(&recaddr, 0, sizeof(recaddr));
  recaddr.sin_family = AF_INET;

  accept(SOCKETFDS[thread_ind], (struct sockaddr*) &recaddr,&len);
  //thread safe because others not calling socket (so no race condition)
  close(SOCKETFDS[thread_ind]);

  //connected successfully
  CONNECTION_STATE[thread_ind] = true;
  // cout << "Connected on rem port: " << ntohs(recaddr.sin_port) << " thread: " <<
  // thread_ind  <<  " socket: " << SOCKETFDS[thread_ind] << endl;

  pthread_exit(NULL);
}

//terminate the threads used to establish connection
void terminateThreads(vector<pthread_t> tid, const string sock_map, string sig){
  //kill and join threads
  int i = 0;

  for(; i < sock_map.size(); ++i){
    if(sock_map[i] == '1'){
      //kill
      if(sig == "SIGUSR2")
        pthread_kill(tid[i], SIGUSR2);
      else
        pthread_kill(tid[i], SIGUSR1);
      //join the thread
      pthread_join(tid[i], NULL);
    }
  }

  //close open sockets only!
  for(i = 0; i < sock_map.size(); ++i){
    if(sock_map[i] == '1')
      close(SOCKETFDS[i]);
  }
}

//check if all connections are connected
bool allConnected(){
  for(int i = 0; i < NUM_PORTS; ++i){
    if(CONNECTION_STATE[i] == false)
      return false;
  }
  return true;
}

/********************
*STARTING TRANSMISSION
********************/
void sendMessage(){
  vector <pthread_t> tid(NUM_PORTS);
  vector<int> pthread_id(NUM_PORTS);

  //initialize thread IDs
  for(int i = 0; i < pthread_id.size(); ++i){
    pthread_id[i] = i;
  }

  //register the signal handler (SIGUSR1)
  registerSignalHandler("SIGUSR1");

  //iterator to binary string to transmit
  auto it = BINARYKEY.begin();
  //*it may be shorter than NUM_PORTS: "10101111" only 8 to send

  //set for closing CLOSE_WAIT sockets from last iteration
  set <int> prev_sockets;
  do{
    //close the sockets again (CLOSE_WAIT)
    for(auto it = prev_sockets.begin(); it != prev_sockets.end(); ++it){
        close(*it);
      }

    // clear sockets for sending
    SOCKETFDS.clear();
    SOCKETFDS.resize(NUM_PORTS, -1);

    //line to send
    cout << "\nsending: " << *it << "\non sockets: ";

    //create new round of sockets
    createSockets(*it);
    printVector(SOCKETFDS);

    //bind them
    bindAndListen(*it);

    for(int i = 0; i < (*it).length(); ++i){
      //send only nonzero elements
      if((*it)[i] == '1'){
        pthread_create((pthread_t *) &(tid[i]), NULL, &threadSend,
            (void *) &pthread_id[i]);
      }
    }

      //main thread wait for sending
      usleep(WAIT_TIME);

      //kill, join and close
      terminateThreads(tid, *it, "SIGUSR1");

      //save previous rounds of sockets for closing
      prev_sockets.insert(SOCKETFDS.begin(),
        SOCKETFDS.end());
      //next line
      ++it;

    }while(it != BINARYKEY.end());
}

void *threadSend(void *arg){

  //accept on port belonging to this thread
  //cast
  int thread_ind = *(int *) arg;

  //struct for accepting
  struct sockaddr_in recaddr;
  socklen_t len = sizeof(recaddr);
  memset(&recaddr, 0, sizeof(recaddr));
  recaddr.sin_family = AF_INET;

  //accept blocks, until client connects or signal kills thread
  accept(SOCKETFDS[thread_ind], (struct sockaddr*) &recaddr,&len);
  //close in thread_terminate

  // cout << "sent!! accessing" << endl;
  // cout << "Sent!!: port: " << ntohs(recaddr.sin_port) << " thread: " <<
  // thread_ind  <<  " socket: " << SOCKETFDS[thread_ind] << endl;
  pthread_exit(NULL);
}

/********************
*UTILITY
********************/
//prints a vector
template<typename T>
void printVector(vector<T> vec){
  for( auto it = vec.begin(); it != vec.end(); ++it){
    cout << *it << " ";
  }
  cout << endl;
}

//print vector of vector
template<typename T>
  void printVecVector(vector<T> vec){
  for(int i = 0; i < vec.size(); ++i){
    //send to printVector
    printVector(vec[i]);
  }
}
