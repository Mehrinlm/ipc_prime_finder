#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9478 /* Port that will be opened */ 
#define BACKLOG 2 /* Number of allowed connections */

main()
{
    
  int fd, fd2; /* file descriptors */

  struct sockaddr_in server; /* server's address information */
  struct sockaddr_in client; /* client's address information */

  int sin_size;


  if ((fd=socket(AF_INET, SOCK_STREAM, 0)) == -1 ){ /* calls socket() */
    printf("socket() error\n");
    exit(-1);
  }

  server.sin_family = AF_INET; 
  server.sin_port = htons(PORT); /* Remember htons() from "Conversions" section? =) */
  server.sin_addr.s_addr = INADDR_ANY; /* INADDR_ANY puts your IP address automatically */ 
  bzero(&(server.sin_zero),8); /* zero the rest of the structure */


  if(bind(fd,(struct sockaddr*)&server,sizeof(struct sockaddr))==-1){ /* calls bind() */
    printf("bind() error\n");
    exit(-1);
  } 

  if(listen(fd,BACKLOG) == -1){ /* calls listen() */
    printf("listen() error\n");
    exit(-1);
  }

  while(1){
    sin_size=sizeof(struct sockaddr_in);
    if ((fd2 = accept(fd,(struct sockaddr *)&client,(socklen_t *)&sin_size))==-1){ /* calls accept() */
      printf("accept() error\n");
      exit(-1);
    }

    printf("You got a connection from %s\n",inet_ntoa(client.sin_addr) ); /* prints client's IP */

    send(fd2,"Welcome to my server.\n",22,0); /* send to the client welcome message */

    close(fd2); /* close fd2 */
  }
}

/*
 * This is currently an ITERATIVE server, to build a concurrent server,
 * a fork is performed after the accept.  The child process closes listenFd,
 * and communicates using connectFd.  The parent process closes connectFd, and
 * then loops back to the accept to wait for another connection request.
 

sockaddr_in serverAddr;
sockaddr &serverAddrCast = (sockaddr &) serverAddr;

// get a tcp/ip socket
// socket() creates the socket
int listenFd = socket(AF_INET, SOCK_STREAM, 0);

bzero(&serverAddr, sizeof(serverAddr));
serverAddr.sin_family = AF_INET;

// any internet interface on this server
serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
serverAddr.sin_port = htons(13);

// gives the address of the socket on the server
bind(listenFd, &serverAddrCast, sizeof(serverAddr));

// specifies the maximum # of connection requests that can be pending for the process
listen(listenFd, 5);

for ( ; ; ) {
    int connectFd = accept(listenFd, (sockaddr *) NULL, NULL);

    // read and write operations on connnectFd
    
    shutdown(connectFd, 2);
    close(connectFd);
}

*/
