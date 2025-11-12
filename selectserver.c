/**
 * 
 * A simple HTTP Server using select().
 * Note that this will only handle the GET requests of HTTP 1.1.
 * 
 * Copyright (c) 2025 - Terabyte Forever
 * 
 */
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>

#define failure(msg) fprintf(stderr,"\033[31;1;4mError:\033[0m %s\n",msg); \
exit(EXIT_FAILURE);

#define debug(msg)
#define debugf(fmt,args...)
#ifdef __DEBUG__
    #undef debug
    #undef debugf
    #define debugf(fmt,...) fprintf(stdout,"\033[38;5;45m[DEBUG]:\033[0m "); \
    fprintf(stdout,fmt,__VA_ARGS__);
    #define debug(msg) fprintf(stdout,"\033[38;5;45m[DEBUG]:\033[0m %s\n",msg);
#endif

#define PORT "8080"

typedef int sock_t;

int maxfd = 0; //maximum fd. this will be used by the select() syscall.

//initialize the socket, make it listenable.
sock_t iSock(){
    int yes = 1; //setsockopt() needs this.
    struct addrinfo hints, *addrinfo_res;
    struct addrinfo* i;
    sock_t hSock = 0;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM; //TCP Socket.
    int gai_res = getaddrinfo(NULL,PORT,&hints,&addrinfo_res);
    if(gai_res != 0){
        failure(gai_strerror(gai_res));
    }
    for(i = addrinfo_res; i != NULL; i = i->ai_next){
        hSock = socket(i->ai_family,i->ai_socktype,i->ai_protocol);
        if(hSock == -1){
            debug("Couldn't create the socket... yet.");
            continue;
        }
        if(setsockopt(hSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1){
            debug("Error while setting up socket options!");
        }
        if(fcntl(hSock,F_SETFL,O_NONBLOCK) == -1){
            debug("Error while setting up the socket to be non blocking!");
        }
        if(bind(hSock,i->ai_addr,i->ai_addrlen) < 0){
            close(hSock);
            debug("Couldn't bind the socket... yet.");
            continue;
        }
        break; //we've found the address if we come down this much, so no need to evaluate any more.
    }
    if(i == NULL){
        //in case that addresses don't exist...
        failure("Failed to bind!");
    }
    freeaddrinfo(addrinfo_res);
    if(listen(hSock,64) == -1){
        failure("Couldn't listen to the socket!");
    }
    debugf("iSock(): Socket created! Listening on port %s.\n",PORT);
    return hSock;
}

//if there is a new client connected...
void new_connection(fd_set* set, sock_t socket){
    sock_t hSock2;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen = sizeof(remoteaddr);
    hSock2 = accept(socket,(struct sockaddr*)&remoteaddr,&addrlen);
    if(hSock2 == -1){
        debug("Error while accept()ing the new fd!");
    }
    else{
        FD_SET(hSock2,set);
        if(hSock2 > maxfd){
            maxfd = hSock2;
        }
        debug("new_connection(): A new client has been connected!");
    }
}
//communicate with the socket.
void sock_com(fd_set* set, sock_t socket){
    char buf[65535];
    if(read(socket,&buf,sizeof(buf)) <= 0){ //if zero bytes are read...
        if(close(socket) == -1){
            debug("Error while closing the socket!");
        }
        FD_CLR(socket,set);
        debug("sock_com(): Connection ended by the remote host!");
    }
    else{
        int index_page = open("./index.html",O_RDONLY);
        uint32_t read_bytes = 0;
        char data[1024] = "HTTP 1.1 200 OK\r\nContent-type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n";
        FILE* fp = fopen("./index.html","r");
        do
        {
            write(socket,data,strlen(data));
            memset(data,0,sizeof(data));
            read_bytes = fread(data,1024,1,fp);
        } while (read_bytes != 0);
        
        //parse that GET request... and send the document.
        //TODO: Implement a table structure like std::map or std::unordered_map to parse HTTP data...
        /*const char http_resp[]  = "<title>It works!</title><h1>Hello, world!</h1>";
        write(socket,http_resp,sizeof(http_resp));*/
        close(socket);
        FD_CLR(socket,set); //we have to close the socket after sending the HTTP response.
    }
}

int main(int argc, char** argv){
    fd_set master;
    fd_set desc;
    FD_ZERO(&desc);
    FD_ZERO(&master);
    sock_t hSock = iSock();
    FD_SET(hSock,&master); //set the master, as it will be handling the fd's.
    maxfd = hSock;
    while(1){
        desc=master;
        if(select(maxfd+1,&desc,NULL,NULL,NULL) == -1){
            failure("select() failed while selecting the appropriate file desc.");
        }
        for(int i = 0; i <= maxfd; i++){
            if(FD_ISSET(i,&desc)){
                if(i == hSock){
                    new_connection(&master,i);
                }
                else{
                    sock_com(&master,i);                  
                }
            }
        }
    }
    return 0;
}