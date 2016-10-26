#include <iostream>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <vector>
#include <cstring>

#define NUMSOCKS 8

using namespace std;

/*
             compiled using g++ -std=c++11 portsweeper.cpp
*/

int main(){
  vector<int> connected;
  char hostname[] = "localhost";
  int reuseaddr = 1;

  
  for(int portno = 1025; portno<=65535; portno++)              
    {


      
      for ( int j = 0; j < NUMSOCKS; j++){

	
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &reuseaddr, sizeof(reuseaddr )) < 0)  //sets the socket option so socket can be 
	  perror("Error setting socket options");                                                      //repeatedly binded to
	
	if (sockfd < 0) {                                                                             //test if the socket was opened
	  perror("ERROR opening socket");
	}


	server = gethostbyname(hostname);                                                            //gethostbyname
	if (server == NULL) {                                                                        // No host found by given hostname
	  fprintf(stderr,"ERROR, no such host\n");
	  exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));                                             //clears out values in serv_addr struct
	serv_addr.sin_family = AF_INET;                                                           
	bcopy((char *)server->h_addr,
	      (char *)&serv_addr.sin_addr.s_addr,
	      server->h_length);
	serv_addr.sin_port = htons(portno + j);
	if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {                     //tries to bind to socket
	  if(errno == EADDRINUSE){
	    printf("[%d]Already with other process\n",portno);                                       // if it is unavailable it will print this statement
	  }
	  else{
	    printf("[%d]could not bind to process (%d) %s\n", portno, errno, strerror(errno));
	  }
	}else{
	  cout<<"portno + j = "<<portno + j<<endl;                                                    //if it is available it will push onto the connected vector
	  connected.push_back(portno + j);
	}

	close(sockfd);

      }

      if (connected.size() == NUMSOCKS)                                                            //if the size of connected vector  == to NUMSOCKS
	{
	  // cout<<"first available = " << connected[0]<<endl;                                       //then we know the NUMSOCKS we need are available
	  return connected[0];}                                                                  // and we return the position of the first portno in that set
      else
	connected.clear();
    }

  return 0;
}
