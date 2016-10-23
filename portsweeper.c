#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>

int main(int argc, char *argv[])
{
    int portno     = 1;
    char *hostname = "localhost";
    //65535
    

    for(portno = 3000; portno<=3009; portno++)
    {
        int sockfd;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("ERROR opening socket");
        }

        server = gethostbyname(hostname);

        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
            serv_addr.sin_port = htons(portno);
        if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
            if(errno == EADDRINUSE){
                printf("[%d]Already with other process\n",portno);
            }
            else{
                printf("[%d]could not bind to process (%d) %s\n", portno, errno, strerror(errno));
            }
        }else{
            printf("[%d]Success\n",portno);
        }
        close(sockfd);
    }
    
    return 0;
}