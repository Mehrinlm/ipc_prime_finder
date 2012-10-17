#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <cstdlib>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> /* needed for struct hostent */

/*
 * TCP/IP Client
 */

#define PORT 9478 /* Open Port on remote Host */
#define MAXDATASIZE 100 /* Max number of bytes of data */

int main(int argc, char *argv[]) {
  int fd, numbytes; /* file descriptors */
  char buf[MAXDATASIZE]; /* buf will stores received text */

  struct hostent *he; /* structure that will get information about the remote host */
  struct sockaddr_in server; /* server's address information */

  if (argc != 2) { /* our program will need one arg (IP) */
    printf("Usage %s<IP Address>\n", argv[0]);
    exit(-1);
  }

  if ((he=gethostbyname(argv[1])) == NULL) { /* calls gethostbyname */
    printf("gethostbyname() error\n");
    exit(-1);
  }

  if ((fd=socket(AF_INET, SOCK_STREAM, 0)) == -1) { /* calls socket */
    printf("socket() error\n");
    exit(-1);
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(PORT); /* htons() is needed again */
  server.sin_addr = *((struct in_addr *)he->h_addr); /*he->h_addr passes "*he's info to h_addr */
  bzero(&(server.sin_zero),8);

  if(connect(fd, (struct sockaddr *)&server,sizeof(struct sockaddr)) == -1) {
    printf("connect() error\n");
    exit(-1);
  }

  if ((numbytes = recv(fd, buf, MAXDATASIZE, 0)) == -1) {
    printf("recv() error\n");
    exit(-1);
  }

  buf[numbytes]='\0';

  printf("Server Messsage: %s\n", buf);

  close(fd);
}
/*
sockaddr_in serverAddr;
sockaddr &serverAddrCast = (sockaddr &)serverAddr;

// get a tcp/ip socket
int sockFd = socket (AF_INET, SOCK_STREAM, 0);

bzero(&serverAddr, sizeof(serverAddr));
serverAddr.sin_family = AF_INET;

// host IP # in dotted decimal format!
inet_pton(AF_INET, serverName, serverAddr.sin_addr);
serverAddr.sin_port = htons(13);

connect(sockFd, serverAddrCast, sizeof(serverAddr));

// read and write operations on sockFd

shutdown(sockFd, 2);
close(sockFd);
*/
