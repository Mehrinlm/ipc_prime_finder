#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <cstdlib>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <iostream>
#include <netdb.h> /* needed for struct hostent */

#define PORT 9478 /* Open Port on remote Host */
#define MAXDATASIZE 100 /* Max number of bytes of data */
#define BACKLOG 2 /* Number of allowed connections */

using namespace std;
vector<int> calc_primes(vector<int> primes);
int getConnectionClient(char* ipAddress);
void send(int socket, vector<int> primes);
int getConnectionServer();
vector<int> recive(int socket);

int main(int argc, char *argv[]){
  int socket;
  int waiting;

	cout << "Is this server? (y or n): ";
  char server;
	cin >> server;
  
  vector<int> primes;
  
  if (server == 'y' || server == 'Y') {
    socket = getConnectionServer();
    waiting = 1;
  } else if (server == 'n' || server == 'N'){
    char ipAddress[12];
    cout << "Enter IP address: ";
    cin >> ipAddress;
    socket = getConnectionClient(ipAddress);
    
    cout << "Min number: ";
    int min;
    cin >> min;
    cout << "Min number: ";
    int max;
    cin >> max;
    
    //populate primes
    for (int i = min; i < max; i++){
      primes.push_back(i);
    }
    waiting = 0;
  } else {
    printf("Incorrect input!\n");
    exit(-1);
  }
	
  while (true){
    if (waiting){
      waiting = 0;
      
      
      
    } else {
      waiting = 1;
      
      
    }
  }
  
}



/** 
 *	Does one iteration and returns all the values remaining in 
 *	the range that have not been ruled out.
 */
vector<int> calc_primes(vector<int> primes) {

	//get the value
	int v = primes[0];

	if (v!=0) {
			//remove all multiples of the value
			int x = v;
			while(x < primes.size()) {
					primes[x]=0;
					x = x+v;
			}
	}
	return primes;
}

int getConnectionClient(char* ipAddress){

  
  
  struct hostent *he = gethostbyname(ipAddress); /* structure that will get information about the remote host */
  struct sockaddr_in server; /* server's address information */
  
  return socket(AF_INET, SOCK_STREAM, 0);
}

void send(int socket, vector<int> primes){
  int bufLocation = 0;
  int size = primes.size();
  char buf[size]; /* buf will stores received text */
  
  for (int i = 0; i < size; i++){
    if (primes[i] != 0){
      buf[bufLocation++] = (char)primes[i];
    }
  }
  buf[bufLocation] = '\0';
  
  send(socket, buf, bufLocation, 0);
  
  
  
/* 
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT); \\ htons() is needed again 
  server.sin_addr = *((struct in_addr *)he->h_addr); \\he->h_addr passes "*he's info to h_addr 
  bzero(&(server.sin_zero),8);
  numbytes = recv(fd, buf, MAXDATASIZE, 0);
  
  //set end of string to '\0'
  buf[numbytes]='\0';
  
  printf("Sent: %s\n", buf);
  */

}

vector<int> recive(int socket){
  char buf[100];
  int numOfBits = recv(socket, buf, 100, 0);
  
  vector<int> primes;
  
  int i = 0;
  while (buf[i] != '\0'){
    int value = (buf[i]);
    primes.push_back(value);
  }
  
  return primes;
  
}

int getConnectionServer(){
  int fd, fd2; /* file descriptors */

  struct sockaddr_in server; /* server's address information */
  struct sockaddr_in client; /* client's address information */

  int sin_size;
  
  fd=socket(AF_INET, SOCK_STREAM, 0);
  
  server.sin_family = AF_INET; 
  server.sin_port = htons(PORT); /* Remember htons() from "Conversions" section? =) */
  server.sin_addr.s_addr = INADDR_ANY; /* INADDR_ANY puts your IP address automatically */ 
  bzero(&(server.sin_zero),8); /* zero the rest of the structure */
  
  bind(fd,(struct sockaddr*)&server,sizeof(struct sockaddr));
  listen(fd,BACKLOG);

    sin_size=sizeof(struct sockaddr_in);
    if ((fd2 = accept(fd,(struct sockaddr *)&client,(socklen_t *)&sin_size))==-1){ /* calls accept() */
      printf("accept() error\n");
      exit(-1);
    }
    
    return fd2;
}
