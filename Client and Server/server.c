#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAXLINE 256
#define AMT_ENTRIES 300

typedef struct sockaddr SA;

FILE* TSLAfd;
FILE* MSFTfd;

typedef struct Entry{
    char date[11];
    double closing;
} Entry;

//given a file descriptor for MSFT.csv or TSLA.csv, returns a pointer to an array of entries
int open_listenfd(char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM; /* Accept connect. */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* …on any IP addr */
    hints.ai_flags |= AI_NUMERICSERV; /* …using numeric port no. */
    getaddrinfo(NULL, port, &hints, &listp);

    /* Walk the list for one that we can bind to */
    for(p = listp; p; p = p->ai_next) {
        /* Createa socketdescriptor*/
        if((listenfd= socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue; /* Socket failed, try the next*/
        }

        /* Eliminates "Address already in use" errorfrom bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

        /* Bindthe descriptor to the address*/
        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
            break; /* Success */
        }
        
        close(listenfd); /* Bind failed, try the next */
    }
    
    // clean up
    freeaddrinfo(listp);
    if (!p) {
        /* No address worked */
        return-1;
    }

    /* Make it a listening socket ready to accept conn. requests */
    // changed backlog argument from LISTENQ constant to 1
    if (listen(listenfd, 1) < 0) {
        // error occurred at listen() call
        close(listenfd);
        return -1;
    }

    return listenfd;
}
float roundToTwo(float x){
  return ((float)(int)(x*100.0) / 100.0);
}
void createEntries(FILE* fd, Entry* entries){
    char buffer[MAXLINE];
    int entriesAmt = 0;
    
    while(fgets(buffer, MAXLINE, fd)){
        
        if(entriesAmt >= 301){
            break;
        }

        if(entriesAmt != 0){
            char* date = NULL;
            char* close = NULL;

            date = strtok(buffer, ",");
            //printf("%s\n", date);
            strtok(NULL, ","); //open
            strtok(NULL, ","); //high
            strtok(NULL, ","); //low
            close = strtok(NULL, ",");
            
            strncpy(entries[entriesAmt-1].date, date, 11);
            entries[entriesAmt-1].closing = atof(close);            
        }

        entriesAmt++;
    }
}

void prices(Entry* entries, char* date, char* buf){
    for(int i = 0; i < AMT_ENTRIES; ++i){
        if(strncmp(entries[i].date, date, 11) == 0){

	    sprintf(buf, "%.2f\n", roundToTwo(entries[i].closing));
            return;
        }
    }
    sprintf(buf, "Unknown\n"); //placeholder
    return;
}
void maxProfit(Entry* entries, char* startDate, char* endDate, char* buf){

  float maxProf = 0;
  float lowest = entries[0].closing;
  float highest = entries[0].closing;
  int startEntry = -1;
  int endEntry = -1;
  for(int i = 0; i < AMT_ENTRIES; ++i){
    if(strncmp(entries[i].date, startDate, 11) == 0){
      startEntry = i;
    }
    if(strncmp(entries[i].date, endDate, 11) == 0){
      endEntry = i;
    }
  }
  if(startEntry == -1 || endEntry == -1){
    sprintf(buf, "Unknown\n");
    return;
  }
  if(startEntry >= endEntry){
    sprintf(buf, "0\n");
    return;
  }
  for(int i = startEntry; i <= endEntry; ++i){
    if(entries[i].closing < lowest){
      lowest = entries[i].closing;
    }
    if(entries[i].closing - lowest > maxProf){
      maxProf = entries[i].closing - lowest;
    }
  }
  sprintf(buf, "%.2f\n", roundToTwo(maxProf));
  return;
}

void echo(int connfd, Entry* MSFTentries, Entry* TSLAentries) {
    size_t n;
    char buf[MAXLINE];
    char header;
    char input[MAXLINE];
    char response[MAXLINE];
    char* args[4];
    int running = 1;

    for(int i = 0; i < MAXLINE; ++i){
      buf[i] = '\0';
    }
   
    while(running == 1 && (n = read(connfd, buf, MAXLINE)) != 0) {

        for(int i = 0; i < MAXLINE; ++i){
	  input[i] = '\0';
	  response[i] = '\0';
	  args[0] = NULL;
	  args[1] = NULL;
	  args[2] = NULL;
	  args[3] = NULL;
        }
        
	header = buf[0];
	//printf("message length: %d, recieved %d bytes\n", header, n);
	//printf("server: header=%c\n", header);
	buf[header] = '\0'; //n-2

	/*for(int i = 0; i < n; ++i){
	  printf("buf:[%d]-%d\n", i, buf[i]);
	  }*/

	
	strncpy(input, buf+1, header); //n-2
	input[header-1] = '\0';

	
	printf("%s\n", input);
	/*
	for(int i = 0; i < n; ++i){
	  printf("input[%d]-%d\n", i, input[i]);
	}
	
	printf("server: input=%s\n", input);
	*/
	args[0] = strtok(input, " \n");
	args[1] = strtok(NULL, " \n");
	args[2] = strtok(NULL, " \n");
	args[3] = strtok(NULL, " \n");

	for(int i = 0; i < MAXLINE; ++i){
	  buf[i] = '\0';
	}
	/*
	for(int i = 0; i < 4; ++i){
	  if(args[i] != NULL){
	    printf("server: args[%d]-%s\n", i, args[i]);
	  }
	}
        */
	if(strcmp(args[0], "List") == 0){
	  strcpy(buf, "TSLA | MSFT\n");
	}
	else if(strcmp(args[0], "Prices") == 0 && args[3] == NULL){
	  if(strcmp(args[1], "MSFT") == 0){
	    prices(MSFTentries, args[2], buf);
	  }
	  else if(strcmp(args[1], "TSLA") == 0){
	    prices(TSLAentries, args[2], buf);
	  }
	  else{
	    strcpy(buf, "Invalid Syntax\n");
	  }
	}
	else if(strcmp(args[0], "MaxProfit") == 0){
	  if(strcmp(args[1], "MSFT") == 0){
	    maxProfit(MSFTentries, args[2], args[3], buf);
	  }
	  else if(strcmp(args[1], "TSLA") == 0){
	    maxProfit(TSLAentries, args[2], args[3], buf);
	  }
	  else{
	    strcpy(buf, "Invalid Syntax\n");
	  }
	}
	else if(strcmp(args[0], "quit") == 0){
	  strcpy(buf, "quit");
	  running = 0;
	}
	else{
	  strcpy(buf, "Invalid Syntax\n");
	}
	
        sprintf(response, "%c%s", (unsigned char)(strlen(buf)), buf);

	
	/*
	printf("server: %d, %c\n", (unsigned int)(strlen(buf)), (unsigned char)(strlen(buf)));
	*/
	//printf("server: response-%s\n", response);
	
	
	write(connfd, response, strlen(response));

	for(int i = 0; i < MAXLINE; ++i){
	  buf[i] = '\0';
	}
    }
}

int main(int argc, char** argv){

    char* MSFTcsvFilename = argv[1];
    char* TSLAcsvFilename = argv[2];
    char* port = argv[3];

    MSFTfd = fopen(MSFTcsvFilename, "r");
    if(MSFTfd == NULL){
        printf("%s not found\n", MSFTcsvFilename);
        return -1;
    }

    TSLAfd = fopen(TSLAcsvFilename, "r");
    if(TSLAfd == NULL){
        printf("%s not found\n", TSLAcsvFilename);
        return -1;
    }

    char stocknames[2][5];
    strncpy(stocknames[0], "MSFT", 5);
    strncpy(stocknames[1], "TSLA", 5);

    Entry TSLAentries[AMT_ENTRIES];
    createEntries(TSLAfd, TSLAentries);

    Entry MSFTentries[AMT_ENTRIES];
    createEntries(MSFTfd, MSFTentries);

    //printf("MSFT date: %s, close: %f\n", MSFTentries[0].date, MSFTentries[0].closing);
    //printf("stockname 1: %s\n", stocknames[0]);


    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough room for any addr */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    listenfd = open_listenfd(port);
    printf("server started\n");
    while (1) {
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        getnameinfo((SA *) &clientaddr, clientlen,
        client_hostname, MAXLINE, client_port, MAXLINE, 0);
        //printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd, MSFTentries, TSLAentries);
        close(connfd);
	break;
    }
    exit(0);


    return 0;
}
