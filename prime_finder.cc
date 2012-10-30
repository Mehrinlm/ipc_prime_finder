/*********************************************************************
 *
 * prime_finder.cc
 * -----------------------------------------------------------------
 *
 * DESCRIPTION
 *  Finds primes with a peer where the user specifies one machine to
 *  be a server, and the other to be a client.  Any execution can be
 *  either client or server.  When calculation is done, both print the
 *  result and terminate.
 *
 * ARGS
 *  (none)
 *
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <sstream>

#define PORT "9328"                                       // the port to bind() / connect() to
#define MAX_IN_LISTEN_QUEUE 10                            // the number of pending connections the server can hold
#define MAX_DATA_SIZE 100                                 // restrict to 100 or less bytes per send

using namespace std;

void receive_removed(int socket_fd, int *removed, int *primes, int *base, int *primes_len, int max);
void remove_multiples(int *primes, int *base, int *max, int *removed, int *removed_count);
int send_all(int socket_connection_fd, int *removed, int count_removed, int *base);
void printOut(char* type, int *primes, int max, int primes_len);
void get_client_socket_fd(int *socket_fd, struct addrinfo *server_info);
void bind_server_to_socket(int *socket_listen_fd);
void accept_client_connection(int *socket_fd, int *socket_listen_fd);
void init_server(int socket_fd, int max, int base);

/*********************************************************************
 *
 * main() execution path
 *
 *********************************************************************/
int main(int argc, char *argv[]) {
  char server;

  int max = 0;              // user-defined upper bounds received by client
  int base = 2;             // base to remove multiples of
  int count_removed = 0;    // the current length of the removed array
  int primes_len = 0;       // the current number of primes remaining

  int *primes;              // list where remaining primes != 0
  int *removed;             // list of items to set to zero in primes

  cout << "Is this server? (y or n): ";
  cin >> server;

  if (server == 'y' || server == 'Y') {

    // -------------------- SERVER BEGIN -------------------- //
    
    int socket_listen_fd;             // socket file descriptor listening for connections
    int socket_fd;                    // socket file descriptor for a connection

    // bind to socket and accept a client
    bind_server_to_socket(&socket_listen_fd);
    accept_client_connection(&socket_fd, &socket_listen_fd);

    // close the listening file descriptor as we've found a client
    close(socket_listen_fd);

    // receive starting max value
    char *max_buf = (char *) calloc(2 * sizeof(int), sizeof(char));
    if (recv(socket_fd, max_buf, 2 * sizeof(int), 0) == -1) {
      perror("server: recv()");
      exit(1);
    }

    // cast incoming data to an int array
    int *int_buf;
    int_buf = (int *)max_buf;

    // convert and assign max value
    max = ntohl(int_buf[0]);

    // convert and assign base value
    base = ntohl(int_buf[1]);

    // free memory from receive
    free(max_buf);

    // instantiate primes array based upon client-defined params
    primes = (int *) malloc(max * sizeof(int));
    for (int i = 0; i < max; i++){
      primes[i] = i;
    }
    primes[1] = 0;                                    // one is not prime and also divides everything evenly
    primes_len = max - 2;

    // keep track of the counts removed and the removed items
    count_removed = 0;
    removed = (int *) calloc((max + 1), sizeof(int));

    // start main calc send & receive loop
    while(base * base < max)  {

      // remove multiples using new base value
      remove_multiples(primes, &base, &max, removed, &count_removed);
      primes_len -= count_removed;

      // determine the next base the client must calculate with
      base++;
      while (base < max && primes[base] == 0) {
        base++;
      }

      // send response to client
      if ((send_all(socket_fd, removed, count_removed, &base)) == -1) {
        perror("server: send_all()");
      }
      
      // receive the list of removed ints from server & update primes
      receive_removed(socket_fd, removed, primes, &base, &primes_len, max);
    }

    // close the connection and free resources
    close(socket_fd);

    // -------------------- SERVER END -------------------- //
  } else {
    // -------------------- CLIENT BEGIN -------------------- //
    
    int socket_fd;                                        // socket file descriptor to write to
    struct addrinfo server_criteria;                      // address info for server
    struct addrinfo *server_info;                         // matched address infos after getaddrinfo() call
    int status;                                           // return value after function calls

    // initalize server_criteria
    memset(&server_criteria, 0, sizeof(server_criteria));
    server_criteria.ai_family = AF_UNSPEC;                // use IPv4 or IPv6
    server_criteria.ai_socktype = SOCK_STREAM;            // use TCP

    // get destination server
    while(1){
      char host[100];
      cout << "Enter host:\n";
      cin >> host;

      // populate server info linked list from criteria
      if ((status = getaddrinfo(host, PORT, &server_criteria, &server_info)) != 0) {
        printf("Host not found, please try again.\n");
        continue;
      }
      break;
    }

    // get socket file descriptor and free server_info
    get_client_socket_fd(&socket_fd, server_info);
    freeaddrinfo(server_info);                            // free server_info list

    // request upper bound from user
    string input = ""; 
    while (input[0] != EOF) {
      cout << "\rEnter the upper bound: ";
      getline(cin, input);
      stringstream ss(input);
      if (ss >> max) {
        break;
      }
      if (input != "") {
        cout << "Not a valid number. Try again.\n";
      } 
    }

    // we can now build arrays that have indices from 0 to user-defined
    // max, so we can remove values by array index
    max++;

    // populate primes and its length var primes_len
    primes = (int *) calloc((max), sizeof(int));
    for (int i = 0; i < max; i++) {
      primes[i] = i;
    }
    primes[1] = 0;                  // one is not prime and also divides everything evenly
    primes_len = max - 2;

    // initialize array of removed elements and its length var count_removed
    removed = (int *) calloc (max + 1, sizeof(int));
    count_removed = 0;

    init_server(socket_fd, max, base);

    // loop until base > sqrt(last number)
    // entering send and recieve loop
    while(base * base < max) {

      // receive the list of removed ints from server & update primes
      receive_removed(socket_fd, removed, primes, &base, &primes_len, max);

      // remove multiples using new base value
      remove_multiples(primes, &base, &max, removed, &count_removed);
      primes_len -= count_removed;

      // determine the next base the server must calculate with
      base++;
      while (base < max && primes[base] == 0) {
        base++;
      }

      // send the next base and array of removed ints
      if ((send_all(socket_fd, removed, count_removed, &base)) == -1) {
        perror("server: send_all()");
      }
    }
    // -------------------- CLIENT END -------------------- //
  }

  // print out the final primes list
  char type[] = "FINAL: ";
  printOut(type, primes, max, primes_len);

  // free primes and removed lists
  free(primes);
  free(removed);

  return 0;
}


/*****************************************************************************
 *
 * init_server()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  sends intial max and base information to the server to intialize
 *  prime-finding
 *
 * ARGS
 *  int *socket_fd              the file descriptor to comm. with server
 *  int max                     the user-defined upper bound + 1
 *  int base                    the value the server should start with
 *  int *socket_listen_fd       the file descriptor to listen on
 *
 ****************************************************************************/
void init_server(int socket_fd, int max, int base) {

  // set initial base value
  int *net_info = (int *) calloc(2, sizeof(int));
  net_info[0] = htonl(max);
  net_info[1] = htonl(base);
  char *info_buf = (char *)net_info;

  // send info to client about data being sent
  if(send(socket_fd, info_buf, sizeof(int) * 2, 0) == -1) {
    perror("server: send()");
    close(socket_fd);
    exit(1);
  }
  free(info_buf);

}

/*****************************************************************************
 *
 * accept_client_connection()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  listens on socket_listen_fd and establishes a connection with the
 *  first available client request.  socket_fd is set as the file
 *  descriptor for communication over this socket
 *
 * ARGS
 *  int *socket_fd              the file descriptor the server acquires for comm.
 *  int *socket_listen_fd       the file descriptor to listen on
 *
 ****************************************************************************/
void accept_client_connection(int *socket_fd, int *socket_listen_fd) {

  struct sockaddr_storage client_addr;                  // client address information
  socklen_t sin_size;

  // start listening on socket
  if (listen(*socket_listen_fd, MAX_IN_LISTEN_QUEUE) == -1) {
    perror("server: listen()");
    exit(1);
  }

  // accept() connections loop
  while (1) {
    sin_size = sizeof(client_addr);

    // accept client request
    if ((*socket_fd = accept(*socket_listen_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
      perror("server: accept()");
      continue;
    }
    
    break;
  }
}

/*****************************************************************************
 *
 * bind_server_to_socket(int *socket_listen_fd)
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  binds server to a socket and sets socket_listen_fd to a file
 *  descriptor to communicate over that socket
 *
 * ARGS
 *  int *socket_listen_fd       the file descriptor the server acquires
 *
 ****************************************************************************/
void bind_server_to_socket(int *socket_listen_fd) {

  struct addrinfo server_criteria;                      // address info for server
  struct addrinfo *server_info;                         // matched address infos after getaddrinfo() call
  struct addrinfo *server_info_iter;                    // points to item in server_info linked list (used to iterate)

  int enable = 1;                                       // used to activate socket reuse in setsockopt()
  int status;                                           // return value after function calls

  // initalize server_criteria
  memset(&server_criteria, 0, sizeof(server_criteria));
  server_criteria.ai_family = AF_UNSPEC;                // use IPv4 or IPv6
  server_criteria.ai_socktype = SOCK_STREAM;            // use TCP
  server_criteria.ai_flags = AI_PASSIVE;                // getaddrinfo() will default to local IP

  // populate server info linked list from criteria
  if ((status = getaddrinfo(NULL, PORT, &server_criteria, &server_info)) != 0) {
    fprintf(stderr, "server: getaddrinfo(): %s\n", gai_strerror(status));
    exit(1);
  }

  // bind() to the first socket possible
  for (server_info_iter = server_info; server_info_iter != NULL; server_info_iter = server_info_iter->ai_next) {

    // obtain the listening socket file descriptor
    if ((*socket_listen_fd = socket(server_info_iter->ai_family, server_info_iter->ai_socktype, server_info_iter->ai_protocol)) == -1) {
      perror("server: socket()");
      continue;                                         // proceed to next addrinfo in linked list
    }

    // allow other sockets to bind() to this port when there is no active listening socket (if program crashes)
    if (setsockopt(*socket_listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
      perror("server: setsockopt()");
      close(*socket_listen_fd);                          // close the file descriptor from socket() call
      continue;                                         // proceed to next addrinfo in linked list
    }

    // bind to the socket
    if (bind(*socket_listen_fd, server_info_iter->ai_addr, server_info_iter->ai_addrlen) == -1) {
      perror("server: bind()");
      close(*socket_listen_fd);                          // close the file descriptor from socket() call
      continue;                                         // proceed to next addrinfo in linked list
    }

    // socket(), setsockopt(), and bind() have all indicated success.  exit loop and use this file descriptor
    break;
  }

  // exit if no succesful bind on a socket
  if (server_info == NULL) {
    fprintf(stderr, "server: unable to bind to socket");
    exit(1);
  }

  freeaddrinfo(server_info);                            // free server_info list
}

/*****************************************************************************
 *
 * get_client_socket_fd()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  sets up a client socket connection with a file descriptor at
 *  argument socket_fd
 *
 * ARGS
 *  int *socket_fd                the file descriptor the client acquires
 *  struct addrinfo* server_info  linked list of socket connection possibilities
 *
 ****************************************************************************/
void get_client_socket_fd(int *socket_fd, struct addrinfo *server_info) {
  struct addrinfo *server_info_iter;                    // points to item in server_info linked list (used to iterate)

  // connect() to the first socket possible
  for (server_info_iter = server_info; server_info_iter != NULL; server_info_iter = server_info_iter->ai_next) {

    // acquire file descriptor for the socket
    if ((*socket_fd = socket(server_info_iter->ai_family, server_info_iter->ai_socktype, server_info_iter->ai_protocol)) == -1) {
      perror("client: socket()");
      continue;
    }

    // connect to the socket
    if (connect(*socket_fd, server_info_iter->ai_addr, server_info_iter->ai_addrlen) == -1) {
      close(*socket_fd);                                 // close the socket file descriptor from socket() call
      perror("client: connect()");
      continue;
    }

    break;
  }

  // exit if no succesful connect() to a socket
  if (server_info == NULL) {
    fprintf(stderr, "client: unable to connect to socket");
    exit(1);
  }
}

/*****************************************************************************
 *
 * printOut()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  prints out non-zero elements of passed in array in format:
 *  
 *    [type]:[comma-delimited first 5 #'s][comma-delimited last 5 #'s]
 *
 * ARGS
 *  char *type        string printed at front of line ('send', 'recv')
 *  int *primes       array of ints to print values from
 *  int max           the length of the array passed in
 *  int primes_len    the number of non-zero elements in that array
 *
 ****************************************************************************/
void printOut(char* type, int *primes, int max, int primes_len){
  if (max == 0){
    printf("%s: No primes deleted\n", type);
    return;
  }
  int *first_primes;
  int *last_primes;
  int half_primes_len = primes_len / 2;
  int nums_limited = half_primes_len > 5 ? 5 : half_primes_len;
  
  int first_primes_len;
  int last_primes_len;

  if (primes_len % 2 == 0) {
    first_primes_len = nums_limited;
    last_primes_len = nums_limited;
  } else {
    first_primes_len = nums_limited;
    last_primes_len = nums_limited + 1;
  }

  first_primes = (int *) calloc (first_primes_len, sizeof(int));
  last_primes = (int *) calloc (last_primes_len, sizeof(int));
  
  int pos = 0;
  for (int i = 0; pos < first_primes_len && i < max; i++){
    if (primes[i] != 0) {
      first_primes[pos] = primes[i];
      pos++;
    }
  }
  
  pos = last_primes_len - 1;
  for (int i = max; pos > -1 && i > -1; i--){
    if (primes[i] != 0) {
      last_primes[pos] = primes[i];
      pos--;
    }
  }

  printf("%s: ", type);
  int num_printed = 0;
  for (int i = 0; i < first_primes_len; i++) {
    if (first_primes[i] != 0) {
      printf(" %d ", first_primes[i]);
    }
  }
  printf(" . . .");
  for (int i = 0; i < last_primes_len - 1; i++) {
    printf(" %d ", last_primes[i]);
  }
  if (last_primes[last_primes_len - 1] > last_primes[last_primes_len - 2]) {
    printf(" %d\n", last_primes[last_primes_len - 1]);
  } else {
    printf("\n");
  }

  free(first_primes);
  free(last_primes);
}

/*****************************************************************************
 *
 * send_all()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  sends updated base value and the array of removed elements to
 *  socket peer
 *
 * ARGS
 *  int socket_connection_fd    socket file descriptor to write to
 *  int *removed                the array to be sent to peer
 *  int count_removed           the length of the removed array to be sent
 *  int *base                   value of which the peer will remove multiples
 *
 ****************************************************************************/
int send_all(int socket_connection_fd, int *removed, int count_removed, int *base) {
  char type[] = "Send";
  printOut(type, removed, count_removed, count_removed);
  int i;
  int *remove_buf = (int *) malloc(sizeof(int) * count_removed);

  for (i = 0; i < count_removed; i++) {
    remove_buf[i] = htonl(removed[i]);
  }

  // cast to chars to send
  char *data;
  data = (char *)&(remove_buf[0]);

  int data_length = sizeof(int) * count_removed;

  int *net_info = (int *) calloc (2, sizeof(int));
  int next_base = *base;
  net_info[0] = htonl(data_length);
  net_info[1] = htonl(next_base);
  char *array_length = (char *)net_info;

  // send info to client about data being sent
  if(send(socket_connection_fd, array_length, sizeof(int) * 2, 0) == -1) {
    perror("server: send()");
    close(socket_connection_fd);
    exit(1);
  }

  int bytes_sent = 0;
  int bytes_remaining = data_length;
  int bytes_this_message;

  while (bytes_sent < data_length) {

    // try to send all bytes
    if((bytes_this_message = send(socket_connection_fd, data + bytes_sent, data_length, 0)) == -1) {
      perror("server: send()");
      break;
    }

    bytes_sent += bytes_this_message;
    bytes_remaining -= bytes_this_message;
  }

  free(remove_buf);
  free(net_info);
  // mirror the functionality of send, return -1 on failure, the total sent bytes on success
  return bytes_this_message == -1 ? bytes_this_message : bytes_sent;
}

/*****************************************************************************
 *
 * remove_multiples()
 * ---------------------------------------------------------------------------
 *
 * DESCRIPTION
 *  removes all multiples of base value in the primes array.
 *  if that value was not previously removed, adds it to the removed
 *  array to send to peer.
 *
 * ARGS
 *  int *primes         the array of remaining primes
 *  int *max            the length of the primes array
 *  int *base           value whose multiples will be removed
 *  int *removed        the array of removed items
 *
 ****************************************************************************/
void remove_multiples(int *primes, int *base, int *max, int *removed, int *removed_count) {

  int localBase = *base;
  int localMax = *max;

	//get the value
  int pos = 0;
  *removed_count = 0;

  //remove all multiples of the value
  int x = 2 * localBase;

  while(x < localMax) {
    if (primes[x] != 0) {
      *(removed + pos) = x;
      pos++;

      primes[x] = 0;
      (*removed_count)++;
    }

    x = x + localBase;
  }

	return;
}

/*********************************************************************
 *
 * received_removed()
 * -----------------------------------------------------------------
 *
 * DESCRIPTION
 *  receives the array of removed items from the socket file descriptor
 *  and updates the primes array
 *
 * ARGS
 *  int socket_fd       the socket file descriptor to read from
 *  int *removed        the array of removed items
 *  int *primes         the array of remaining primes
 *  int *base           value whose multiples will be removed
 *  int *primes_len     the number of remaining primes
 *  int max             the length of the primes array
 *
 *********************************************************************/
void receive_removed(int socket_fd, int *removed, int *primes, int *base, int *primes_len, int max) {
  char data_length_buffer[sizeof(int) * 2];

  if ((recv(socket_fd, data_length_buffer, sizeof(int) * 2, 0)) == -1) {
      perror("client: recv()");
      exit(1);
  }

  int *int_list;
  int_list = (int *)data_length_buffer;


  int data_length = ntohl(int_list[0]);
  *base = ntohl(int_list[1]);

  int bytes_received = 0;
  int bytes_remaining = data_length;
  int bytes_this_message;
  int array_length = data_length / sizeof(int);
  char *byte_data = (char *)calloc(data_length, sizeof(char));

  while (bytes_received < data_length) {

    // receive bytes in recv() calls of length MAX_DATA_SIZE
    if ((bytes_this_message = recv(socket_fd, byte_data + bytes_received, MAX_DATA_SIZE, 0)) == -1) {
        perror("client: recv()");
        exit(1);
    }

    bytes_received += bytes_this_message;
    bytes_remaining -= bytes_this_message;

  }
  
  // populate removed array and update primes list
  
  int *tempholder = (int *)byte_data;
  for (int i = 0; i < array_length; i++) {
    removed[i] = ntohl(tempholder[i]);
    primes[removed[i]] = 0;
  }
  free(tempholder);
  // update global primes_len
  *primes_len -= array_length;
  char type[] = "Recv";
  printOut(type, removed, array_length, array_length);
  
}

