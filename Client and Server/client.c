#define _POSIX_C_SOURCE 201712L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>


#define MAX_LINE_SIZE 256

typedef struct sockaddr SA;

int open_clientfd(char *hostname, char *port) {
  int clientfd;
  struct addrinfo hints, *listp, *p;
  /* Get a list of potential server addresses */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM; /* Open a connection */
  hints.ai_flags = AI_NUMERICSERV; /* ...using numeric port arg. */
  hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
  getaddrinfo(hostname, port, &hints, &listp);
  /* Walk the list for one that we can successfully connect to */
  for (p = listp; p; p = p->ai_next) {
    /* Create a socket descriptor */
    if ((clientfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) < 0)
      continue; /* Socket failed, try the next */
    /* Connect to the server */
    if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
      break; /* Success */
    close(clientfd); /* Connect failed, try another */
  }
  /* Clean up */
  freeaddrinfo(listp);
  if (!p) /* All connects failed */
    return -1;
  else /* The last connect succeeded */
    return clientfd;
}


int main(int argc, char** argv){

  int clientfd;
  char* host;
  char* port;

  char header = 0;
  host = argv[1];
  port = argv[2];

  char buf[MAX_LINE_SIZE];
  char buf2[MAX_LINE_SIZE];

  clientfd = open_clientfd(host, port);
  if(clientfd != -1){
    //printf("connection made\n");
  }
  
  int running = 1;
  while(running){
    printf("> ");

    fgets(buf, MAX_LINE_SIZE, stdin);
    sprintf(buf2, "%c%s", (unsigned char)((unsigned int)(strlen(buf))), buf);

    //printf("client: sending:%s\n", buf2);
    
    write(clientfd, buf2, strlen(buf2));
    if(strcmp(buf, "quit\n") == 0){
	running = 0;
	break;
    }
    read(clientfd, buf, MAX_LINE_SIZE);
    header = buf[0];
    buf[header+1] = '\0';
    
    fputs(buf, stdout);
    for(int i = 0; i< MAX_LINE_SIZE; ++i){
      buf[i] = '\0';
      buf2[i] = '\0';
    }
  }
  close(clientfd);
  exit(0);
}
