/************
  Compile :  make client
  Run: ./blink [server IP] [num_ports] [wait_time] [saved_file]
  or make runClient

  Requires running server

  It finds NUM_PORTS contiguous ports. Listens for client. Begins transmission.
************/



#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <fstream>

  using namespace std;

/********************
*GLOBAL VARIABLES:
********************/
string IP = "192.168.0.102";
int NUM_PORTS = 8;
int START_PORT = 1025;          //begin port search
int WAIT_TIME = 2;
string SAVED = "bin_file.txt";  //location to save received message
int CONNECT_ATTEMPT = 2;


vector<int> SOCKETFDS;                   //contains socket file descriptors
vector<bool> CONNECTION_STATE;           //state of each port
vector<struct addrinfo *> RESSAVE_PTRS;  //hold link list pointer getaddrinfo
vector<struct addrinfo *> RESCUR_PTRS;   //hold current res pointer
vector <string> BINARYKEY;               //holds the key
vector <string> CLEAN_BINARYKEY;         //holds the key
string RECEIVED_MAP;

/********************
*FUNCTIONS:
********************/
//SETUP
void parseArgs(int argc, char** argv);

//INIATE CONNECTION WITH SERVER
int findServerPorts();
void registerSignalHandler();
void createSockets();
void *threadConnect(void *arg);
void terminateThreads(vector<pthread_t> tid);

//RECEIVING
void startReceiving();
void *threadReceive(void *arg);
bool isStopped();

void writeBinary();
//void convertBinarytoKey();

//SIGNAL HANDLERS
void threadConnectSignalHandler(int signo){
  //exit, so thread can join
  cout << "entered signal handler!!" << endl;

  for(int i = 0; i < RESSAVE_PTRS.size(); ++i){
    if(RESSAVE_PTRS[i] != NULL){
      freeaddrinfo(RESSAVE_PTRS[i]);
      RESSAVE_PTRS[i] = NULL;
    }
  }
  pthread_exit(NULL);
}


//UTILITY
template<typename T>
  void printVector(vector<T> vec);

template<typename T>
  void printVecVector(vector<T> vec);


int main(int argc, char** argv){

  cout << "\nClient Starting..." << endl;
  //parse arguments: server_ip, num_ports
  parseArgs(argc, argv);

  //find server port
  cout << "Finding Server... " << endl;
  START_PORT = findServerPorts();
  cout << "Connected to Server on Start Port : " << START_PORT << endl << endl;;

  cout << "Listening for Message..." << endl;
  startReceiving();

  cout << "Writing Message to File..." << endl;
  writeBinary();
  return 0;
}

/********************
*PARSE
********************/
void parseArgs(int argc, char** argv){
  //need at least 3 arguments: NUM_PORTS and 1 path
  if(argc < 5){
    cerr << "Error client.cpp: not enough Arguments\n" <<
    "Usage: ./client [server_ip] [num_ports] [wait_time]\
    [saved_file]" << endl;
    exit(1);
  }

  IP = argv[1];
  NUM_PORTS = stoi(argv[2]);
  WAIT_TIME = stoi(argv[3]);
  SAVED = argv[4];
  SOCKETFDS = vector<int> (NUM_PORTS);
  RESSAVE_PTRS = vector<struct addrinfo *> (NUM_PORTS);
  RESCUR_PTRS = vector<struct addrinfo *> (NUM_PORTS);

  if(NUM_PORTS % 8 !=0){
    cerr << "Error client.cpp: NUMPORTS not multiple of 8" << endl;
    exit(1);
  }

 cout << "IP = " << IP << " NUM_PORTS = " << NUM_PORTS <<
 " WAIT_TIME = " << WAIT_TIME <<  endl;
}


/********************
*ESTABLISH CONNECTION
********************/
//finds the ports the server is using
int findServerPorts(){
    //keep it simple at first!!!!
    vector <pthread_t> tid(NUM_PORTS);
    vector<int> thread_id(NUM_PORTS);
    int orig_start_port = START_PORT;

    //thread identifiers
    for(int i = 0; i < NUM_PORTS; ++i){
      thread_id[i] = i;
    }

    //register thread signal handler
    registerSignalHandler();

    set<int> prev_sockets;

    //last ports checked 65527 - 65535
    for(int portnum = orig_start_port; portnum < 65536 - NUM_PORTS; ++portnum){
      START_PORT = portnum;

      //closing previous sockets fin_wait
      for(auto it = prev_sockets.begin(); it != prev_sockets.end(); ++it){
        close(*it);
      }

      //clear previous socket values
      SOCKETFDS.clear();
      SOCKETFDS.resize(NUM_PORTS,-1);

      //create sockets for each thread: SOCKETFDS
      createSockets();
      for(int i = 0; i < NUM_PORTS; ++i){
        //each thread attempts to connect
        pthread_create((pthread_t *) &(tid[i]), NULL, &threadConnect,
                        (void *) &thread_id[i]);
      }

      //main thread waits a few seconds (like server)
      usleep(WAIT_TIME);

      //kills, joins and closes sockets
      terminateThreads(tid);

      //saves previous sockets (helps close previous sockets)
      prev_sockets.insert(SOCKETFDS.begin(),SOCKETFDS.end());

      //if all are connected, found startport !!
      bool allConnected = true;
      for(int i = 0; i < NUM_PORTS; ++i){
        if(SOCKETFDS[i] == -1){
          cout << "Unable to connect starting at port: " << START_PORT << endl;
            allConnected = false;
            break;
        }
      }
      //success!
      if(allConnected){
        return portnum;
      }

    }//out of ports to check
    return -1;
}

//attempts connect thread
void *threadConnect(void *arg){
  //success returns not -1 in SOCKETFDS[thread_id]

  //get arg
  int thread_id = *(int *) arg;
  //attempt to connect CONNECT_ATTEMPT times
  int attempts = CONNECT_ATTEMPT;
  do{
    if(connect(SOCKETFDS[thread_id], RESCUR_PTRS[thread_id]->ai_addr,
       RESCUR_PTRS[thread_id]->ai_addrlen) == 0){
      //success!!
      //terminateThreads will close all sockets
      pthread_exit(NULL);
    }
    --attempts;
  }while(attempts != 0);

  //connection not successful
  SOCKETFDS[thread_id] = -1;

  pthread_exit(NULL);
}

//kill, join, close and freeaddrinfo for all threads
void terminateThreads(vector<pthread_t> tid){
  //kill and join threads
  int i = 0;
  //kill NUM_PORTS thread
  for(; i < NUM_PORTS; ++i){
    //kill
    pthread_kill(tid[i], SIGUSR2);
    //join the thread
    pthread_join(tid[i], NULL);
  }

  //close NUM_PORTS because NUM_PORTS sockets created
  for(i = 0; i < NUM_PORTS; ++i){
    if(SOCKETFDS[i] != -1)
      close(SOCKETFDS[i]);
  }

  //freeaddrinfo for
  for(i = 0; i < RESSAVE_PTRS.size(); ++i){
    freeaddrinfo(RESSAVE_PTRS[i]);
    RESSAVE_PTRS[i] = NULL;
    RESCUR_PTRS[i] = NULL;
  }
}


//signal handler for pthreads
void registerSignalHandler(){
  //signal handler for SIGUSR2
  struct sigaction sig_struct;
  //clear the signal set
  sigemptyset(&sig_struct.sa_mask);
  sig_struct.sa_flags = 0;

  sigaction(SIGUSR2, &sig_struct, 0);
  sig_struct.sa_handler = threadConnectSignalHandler;
}

void createSockets(){

  for(int i = 0; i < SOCKETFDS.size(); ++i){
    //create struct
    struct addrinfo hints, *res;

    memset((void *) &hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(IP.c_str(),to_string(START_PORT+i).c_str(),&hints,&res);

    //save it for later freeing
    RESSAVE_PTRS[i] = res;
    int reuseaddr = 1;
    do{
      //for each SOCKETFDS
      SOCKETFDS[i] = socket(res->ai_family, res->ai_socktype,
        res->ai_protocol);

      //set socket options
      setsockopt(SOCKETFDS[i], SOL_SOCKET, SO_REUSEADDR,
        (char *) &reuseaddr, sizeof(reuseaddr));
      setsockopt(SOCKETFDS[i], SOL_SOCKET, SO_REUSEPORT,
        (char *) &reuseaddr, sizeof(reuseaddr));

      //didn't get good socket?
      if(SOCKETFDS[i] < 0)
          continue;
      else{
        RESCUR_PTRS[i] = res;
        break;
      }
    }while((res = res->ai_next) != NULL);
  }

}

/********************
*START RECEIVING
********************/
void startReceiving(){
  vector <pthread_t> tid(NUM_PORTS);
  vector<int> thread_id(NUM_PORTS);

  for(int i = 0; i < NUM_PORTS; ++i){
    thread_id[i] = i;
  }

  //again uses SIGUSR1
  registerSignalHandler();

  //save all previous sockets!
  set<int> prev_sockets;
  do{
    //keep closing previous sockets
    for(auto it = prev_sockets.begin(); it != prev_sockets.end(); ++it){
      close(*it);
    }

    //clear sockets
    SOCKETFDS.clear();
    SOCKETFDS.resize(NUM_PORTS, -1);

    //new line
    RECEIVED_MAP.clear();
    RECEIVED_MAP.resize(NUM_PORTS, '0');

    createSockets();

    for(int i = 0; i < NUM_PORTS; ++i){
      pthread_create((pthread_t *) &(tid[i]), NULL, &threadReceive,
                      (void *) &thread_id[i]);
    }

    usleep(WAIT_TIME);

    terminateThreads(tid);

    cout << "\nreceived: " << RECEIVED_MAP;
    //push it!
    BINARYKEY.push_back(RECEIVED_MAP);

    cout  << "\non sockets: ";
    for(int i = 0; i < SOCKETFDS.size(); ++i){
      cout << SOCKETFDS[i] << " ";
    }
    cout << endl;

    prev_sockets.insert(SOCKETFDS.begin(),SOCKETFDS.end());

  }while(!isStopped());

}

void *threadReceive(void *arg){
  //get arg
  int thread_id = *(int *) arg;

  //sockets are already created

  int attempts = 4;
  //connect
  do{
    //socket, connect and close may not be multi-thread safe
    //writing to diff element in SOCKETFDS is thread safe!

    if(connect(SOCKETFDS[thread_id], RESCUR_PTRS[thread_id]->ai_addr,
      RESCUR_PTRS[thread_id]->ai_addrlen) == 0){
      //success!!

      //SET TO 1
      /////////////////////////////
      RECEIVED_MAP[thread_id] = '1';
      /////////////////////////////
      pthread_exit(NULL);

    }
   --attempts;
 }while(attempts !=0 );
  //connection not successful! RECEIVED_MAP == 0 already

  pthread_exit(NULL);
}

//server stopped? (every NUM_PORTS multiple port should be 0 to stop)
bool isStopped(){
  //check if still transmitting
  for(int i = 0; i < RECEIVED_MAP.length(); ++i){
    if(RECEIVED_MAP[i] == '1')
      return false;
  }
  return true;
}

//writes the key in binary SAVEDFILE
void writeBinary(){
  //open file
  ofstream savefile;
  savefile.open(SAVED);
  for(auto it = BINARYKEY.begin(); it != BINARYKEY.end(); ++it){
      int div = (*it).length()/8;
      for(int i = 0; i < div; ++i){
        savefile << (*it).substr(i*8,8) << endl;
        //CLEAN_BINARYKEY.push_back((*it).substr(i*8,8));
      }
  }
  savefile.close();
}

/********************
*CONVERT
********************/

/********************
*UTILITY
********************/
//prints a vector
template<typename T>
  void printVector(vector<T> vec){
  for( auto it = vec.begin(); it != vec.end(); ++it){
    cout << " " << *it;
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
