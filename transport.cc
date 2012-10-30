
/*
 * transport.cc -- client or server to communicate
 */

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
#define ACK '~'                                           // acknowledgement that communication is done

#define DEBUG 0
#define INTEGRATED 0
#define BASE 0

using namespace std;

void reap_all_dead_child_processes(int signal) {
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

int send_all(int socket_connection_fd, int *removed, int count_removed, int *base) {

  int i;
  if (DEBUG) {
    printf("\nin send_all() (before htonl)\n\nValues are:");
    for (i = 0; i < count_removed; i++) {
      printf("%d ", removed[i]);
    }
  }

  if (DEBUG) {
    printf("Sending removed number count rmd: %d\n", count_removed);
  }
  int *remove_buf = (int *) malloc(sizeof(int) * count_removed);

  for (i = 0; i < count_removed; i++) {
    remove_buf[i] = htonl(removed[i]);
  }

  // cast to chars to send
  char *data = (char *)calloc(sizeof(int) * count_removed, 1);
  data = (char *)&(remove_buf[0]);

  int data_length = sizeof(int) * count_removed;

  if (DEBUG) {
    printf("Sending: data_len: %d\n", data_length);
    printf("Sendinf: base(drf): %d\n", *base);
  }

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

  // mirror the functionality of send, return -1 on failure, the total sent bytes on success
  return bytes_this_message == -1 ? bytes_this_message : bytes_sent;
}

/**
 *  Prints out what was just passed in in correc format
 */
void printOut(char* type, int *primes, int max, int primes_len){

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
  for (int i = 0; pos < first_primes_len && i < half_primes_len; i++){
    if (primes[i] != 0) {
      first_primes[pos] = primes[i];
      pos++;
    }
  }
  
  pos = last_primes_len - 1;
  for (int i = primes_len; pos > -1 && i >= half_primes_len; i--){
    if (primes[i] != 0) {
      last_primes[pos] = primes[i];
      pos--;
    }
  }

  printf("%s: ", type);
  int num_printed = 0;
  for (int i = 0; i < 10; i++) {
    if (first_primes[i] != 0) {
      printf(" %d,", first_primes[i]);
    }
  }
  printf(" . . .");
  for (int i = 0; i < 9; i++) {
    if (last_primes[i] != 0) {
      printf(" %d,", last_primes[i]);
    }
  }
  printf(" %d\n", last_primes[9]);

  free(first_primes);
  free(last_primes);
}

/**
 * Calculates if all primes are done, if so returns false
 * 
 */
int isNotDone(vector<int> primes, int start){
  if (start == 0) return 1;
  return ((primes.size()-1) == start);
}

/** 
 *	Does one iteration and returns all the values remaining in 
 *	the range that have not been ruled out.
 */
void remove_multiples(int *primes, int *base, int *max, int *removed, int *removed_count) {
  if (DEBUG) {
    printf("max = %d\n", *max);
    printf("base = %d\n", *base);
    printf("cnt rmd = %d\n", *removed_count);
    printf("Pos\tRemoved");
    for (int i = 0; i < *removed_count; i++) {
      printf("%d\t%d\n", i, removed[i]);
    }
    printf("Pos\tListItem");
    for (int i = 0; i < *max; i++) {
      printf("%d\t%d\n", i, primes[i]);
    }
  }

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
  
  if (DEBUG) {
    for (int i = 0; i < *removed_count; i++) {
      printf("removed[%d]: %d\n\n\n:", removed[i]);
    }
  }

  printf("in remove multiples: removed_count: %d\n", *removed_count);

	return;
}

/**
 * receives the array of removed items from the socket file descriptor
 */
void receive_removed(int socket_fd, int *removed, int *primes, char *ack, int *base, int *primes_len) {
  char data_length_buffer[sizeof(int) * 2];

  if ((recv(socket_fd, data_length_buffer, sizeof(int) * 2, 0)) == -1) {
      perror("client: recv()");
      exit(1);
  }

  int *int_list = (int *)malloc(sizeof(int) *2);
  int_list = (int *)data_length_buffer;


  int data_length = ntohl(int_list[0]);
  *base = ntohl(int_list[1]);

  printf("Received: data_length: %d\n", data_length);
  printf("base = %d\n", *base);

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

  removed = (int *)byte_data;
  for (int i = 0; i < array_length; i++) {
    removed[i] = ntohl(removed[i]);
    primes[removed[i]] = 0;
  }

  printf("Pos\tRemoved\n");
  for (int i = 0; i < array_length; i++) {
    printf("%d\t%d\n", i, removed[i]);
  }

  printf("array_length: %d\n", array_length);
  *primes_len -= array_length;
  printf("primes_len: %d\n", *primes_len);
}

void print_status(int *max, int *base, int *count_removed, int *removed, int * primes) {
  if (DEBUG) {
    printf("max = %d\n", *max);
    printf("base = %d\n", *base);
    printf("cnt rmd = %d\n", *count_removed);
    printf("Pos\tRemoved");
    for (int i = 0; i < *count_removed; i++) {
      printf("%d\t%d\n", i, removed[i]);
    }
    printf("Pos\tListItem");
    for (int i = 0; i < *max; i++) {
      printf("%d\t%d\n", i, primes[i]);
    }
  } else if (BASE) {
    printf("base = %d\n", *base);
  }
}

void calc_loop(int socket_fd, int *removed, int *primes, char *ack, int *base,
        int *max, int *count_removed, int *primes_len) {

  while(*base < *max)  {
    remove_multiples(primes, base, max, removed, count_removed);

    if (DEBUG) {
      printf("max = %d\n", *max);
      printf("base = %d\n", *base);
      printf("cnt rmd = %d\n", *count_removed);
      printf("Pos\tRemoved");
      for (int i = 0; i < *count_removed; i++) {
        printf("%d\t%d\n", i, removed[i]);
      }
      printf("Pos\tListItem");
      for (int i = 0; i < *max; i++) {
        printf("%d\t%d\n", i, primes[i]);
      }
    } else if (BASE) {
      printf("base = %d\n", *base);
    }

    // send response to client
    if ((send_all(socket_fd, removed, *count_removed, base)) == -1) {
      perror("server: send_all()");
    }

    receive_removed(socket_fd, removed, primes, ack, base, primes_len); 

    if (DEBUG) {
      printf("max = %d\n", *max);
      printf("base = %d\n", *base);
      printf("cnt rmd = %d\n", *count_removed);
      printf("Pos\tRemoved");
      for (int i = 0; i < *count_removed; i++) {
        printf("%d\t%d\n", i, removed[i]);
      }
      printf("Pos\tListItem");
      for (int i = 0; i < *max; i++) {
        printf("%d\t%d\n", i, primes[i]);
      }
    } else if (BASE) {
      printf("base = %d\n", *base);
    }
  }
}

int main(int argc, char *argv[]) {
  char server;

  cout << "Is this server? (y or n): ";
  cin >> server;

  if (server == 'y' || server == 'Y') {

    // -------------------- SERVER BEGIN -------------------- //
    
    int socket_listen_fd;                                 // socket file descriptor listening for connections
    int socket_connection_fd;                             // socket file descriptor for a connection

    struct addrinfo server_criteria;                      // address info for server
    struct addrinfo *server_info;                         // matched address infos after getaddrinfo() call
    struct addrinfo *server_info_iter;                    // points to item in server_info linked list (used to iterate)
    struct sockaddr_storage client_addr;                  // client address information
    socklen_t sin_size;

    struct sigaction sa;                                  // signal handler action for killing off zombie processes
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
      return 1;
    }

    // bind() to the first socket possible
    for (server_info_iter = server_info; server_info_iter != NULL; server_info_iter = server_info_iter->ai_next) {

      // obtain the listening socket file descriptor
      if ((socket_listen_fd = socket(server_info_iter->ai_family, server_info_iter->ai_socktype, server_info_iter->ai_protocol)) == -1) {
        perror("server: socket()");
        continue;                                         // proceed to next addrinfo in linked list
      }

      // allow other sockets to bind() to this port when there is no active listening socket (if program crashes)
      if (setsockopt(socket_listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("server: setsockopt()");
        close(socket_listen_fd);                          // close the file descriptor from socket() call
        continue;                                         // proceed to next addrinfo in linked list
      }

      // bind to the socket
      if (bind(socket_listen_fd, server_info_iter->ai_addr, server_info_iter->ai_addrlen) == -1) {
        perror("server: bind()");
        close(socket_listen_fd);                          // close the file descriptor from socket() call
        continue;                                         // proceed to next addrinfo in linked list
      }

      // socket(), setsockopt(), and bind() have all indicated success.  exit loop and use this file descriptor
      break;
    }

    // exit if no succesful bind on a socket
    if (server_info == NULL) {
      fprintf(stderr, "server: unable to bind to socket");
      return 2;
    }

    freeaddrinfo(server_info);                            // free server_info list

    // start listening on socket
    if (listen(socket_listen_fd, MAX_IN_LISTEN_QUEUE) == -1) {
      perror("server: listen()");
      exit(1);
    }

    // setup signal handler for forked child processes
    sa.sa_handler = reap_all_dead_child_processes;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;                             // resume interruptable reads and writes on file descriptors
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("server: sigaction() failed to set SIGCHLD handler");
    }

    // accept() connections loop
    while (1) {
      sin_size = sizeof(client_addr);

      // accept client request
      if ((socket_connection_fd = accept(socket_listen_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
        perror("server: accept()");
        continue;
      }

      //int child_pid;
      //if ((child_pid == fork()) == 0) {

        // ---- CHILD PROCESS BEGIN ----//

        //close(socket_listen_fd);                          // close child listening file descriptor

        // receive starting max value
        char *max_buf = (char *) calloc(2 * sizeof(int), sizeof(char));
        if (recv(socket_connection_fd, max_buf, 2 * sizeof(int), 0) == -1) {
          perror("server: recv()");
          exit(1);
        }

        // cast incoming data to an int
        int *int_buf = (int *)malloc(sizeof(int) * 2);
        int_buf = (int *)max_buf;

        // convert and assign max value
        int *max = (int *) calloc(1, sizeof(int));
        *max = ntohl(int_buf[0]);

        int primes_len = *max - 2;

        // convert and assign base value
        int *base = (int *) calloc(1, sizeof(int));
        *base = ntohl(int_buf[1]);

        // instantiate primes array based upon client-defined params
        int *primes = (int *) malloc((*max) * sizeof(int));
        for (int i = 0; i < *max; i++){
          primes[i] = i;
        }
        primes[1] = 0;                                    // one is not prime and also divides everything evenly

        // keep track of the counts removed and the removed items
        int *count_removed = (int *) calloc(1, sizeof(int));
        int *removed = (int *) calloc((*max + 1), sizeof(int));
        
        if (DEBUG) {
          print_status(max, base, count_removed, removed, primes);
        } else if (BASE) {
          printf("base = %d\n", *base);
        }

        char *ack = (char *) calloc(1, sizeof(char));

        // ---- START LOOPING BEHAVIOR NOW ---- //

        if (INTEGRATED) {
          calc_loop(socket_connection_fd, removed, primes, ack, base, max, count_removed, &primes_len);
        } else {
          while(*base < *max)  {
            remove_multiples(primes, base, max, removed, count_removed);
            primes_len -= *count_removed;

            printf("\n\nafter remove_mults: primes_len : %d\n", primes_len);
            printf("REMAINING PRIMES:\n-----------------\n");
            for (int i = 0; i < *max; i++) {
              if (primes[i] != 0) {
                printf("%d, ", primes[i]);
              }
            }

            printf("Pos\tRemoved");
            for (int i = 0; i < *count_removed; i++) {
              printf("%d\t%d\n", i, removed[i]);
            }

            if (DEBUG) {
              printf("max = %d\n", *max);
              printf("base = %d\n", *base);
              printf("cnt rmd = %d\n", *count_removed);
              printf("Pos\tRemoved");
              for (int i = 0; i < *count_removed; i++) {
                printf("%d\t%d\n", i, removed[i]);
              }
              printf("Pos\tListItem");
              for (int i = 0; i < *max; i++) {
                printf("%d\t%d\n", i, primes[i]);
              }
            } else if (BASE) {
              printf("base = %d\n", *base);
            }

            (*base)++;
            while (*base < *max && primes[*base] == 0) {
              (*base)++;
            }

            // send response to client
            if ((send_all(socket_connection_fd, removed, *count_removed, base)) == -1) {
              perror("server: send_all()");
            }
           
            if (DEBUG) {
              printf("BETWEENZEES");
              printf("Pos\tRemoved");
              for (int i = 0; i < *count_removed; i++) {
                printf("%d\t%d\n", i, removed[i]);
              }
            } else if (BASE) {
              printf("base = %d\n", *base);
            }

            receive_removed(socket_connection_fd, removed, primes, ack, base, &primes_len);

            printf("\n\nafter receive: primes_len : %d\n", primes_len);
            printf("REMAINING PRIMES:\n-----------------\n");
            for (int i = 0; i < *max; i++) {
              if (primes[i] != 0) {
                printf("%d, ", primes[i]);
              }
            }

            
            if (DEBUG) {
              printf("I ASSUME IT IS HERE");
              printf("max = %d\n", *max);
              printf("base = %d\n", *base);
              printf("cnt rmd = %d\n", *count_removed);
              printf("Pos\tRemoved");
              for (int i = 0; i < *count_removed; i++) {
                printf("%d\t%d\n", i, removed[i]);
              }
              printf("Pos\tListItem");
              for (int i = 0; i < *max; i++) {
                printf("%d\t%d\n", i, primes[i]);
              }
            } else if (BASE) {
              printf("base = %d\n", *base);
            }
          }
        }

        /*
        // wait to close the file descriptor until we have acknowledgement that the client is done
        memset(ack, 0, sizeof(char));
        while (ack[0] != ACK) {

          if (recv(socket_connection_fd, ack, sizeof(char), 0) == -1) {
              perror("server: recv() ACK");
              exit(1);
          }
        }
        */

        close(socket_connection_fd);                      // close as request has been handled
        //return 0;                                         // terminate child process
/*
        // ---- CHILD PROCESS END ----//
      } else {
        if (DEBUG) {
          printf("parent process?");
        }
        int status;
        waitpid(child_pid, &status, 0);
      }
      */
    }

    return 0;

    // -------------------- SERVER END -------------------- //
  } else {

    // -------------------- CLIENT BEGIN -------------------- //
    
    int socket_fd;                                        // socket file descriptor to write to
    char buffer[MAX_DATA_SIZE];                           // receiving buffer limited to MAX_DATA_SIZE
    int *int_list;                                        // the list of remaining numbers

    struct addrinfo server_criteria;                      // address info for server
    struct addrinfo *server_info;                         // matched address infos after getaddrinfo() call
    struct addrinfo *server_info_iter;                    // points to item in server_info linked list (used to iterate)

    int status;                                           // return value after function calls

    // initalize server_criteria
    memset(&server_criteria, 0, sizeof(server_criteria));
    server_criteria.ai_family = AF_UNSPEC;                // use IPv4 or IPv6
    server_criteria.ai_socktype = SOCK_STREAM;            // use TCP

    // get destination server
    // TODO: might have to change this char array thing
    char host[100];
    cout << "Enter host:\n";
    cin >> host;

    // populate server info linked list from criteria
    if ((status = getaddrinfo(host, PORT, &server_criteria, &server_info)) != 0) {
      fprintf(stderr, "client: getaddrinfo(): %s\n", gai_strerror(status));
      return 1;
    }

    // connect() to the first socket possible
    for (server_info_iter = server_info; server_info_iter != NULL; server_info_iter = server_info_iter->ai_next) {

      // acquire file descriptor for the socket
      if ((socket_fd = socket(server_info_iter->ai_family, server_info_iter->ai_socktype, server_info_iter->ai_protocol)) == -1) {
        perror("client: socket()");
        continue;
      }

      // connect to the socket
      if (connect(socket_fd, server_info_iter->ai_addr, server_info_iter->ai_addrlen) == -1) {
        close(socket_fd);                                 // close the socket file descriptor from socket() call
        perror("client: connect()");
        continue;
      }

      break;
    }

    // exit if no succesful connect() to a socket
    if (server_info == NULL) {
      fprintf(stderr, "client: unable to connect to socket");
      return 2;
    }

    freeaddrinfo(server_info);                            // free server_info list

    // request upper bound from user
    string input = ""; 
    int *max = (int *) malloc(sizeof(int));
    while (input[0] != EOF) {
      cout << "Enter the upper bound: ";
      getline(cin, input);
      stringstream ss(input);
      if (ss >> *max) {
        break;
      }
      cout << "Not a valid number. Try again.\n";
    }

    int *primes = (int *) calloc((*max + 1), sizeof(int));
    (*max)++;

    // populate primes
    for (int i = 0; i < *max; i++) {
      primes[i] = i;
    }
    primes[1] = 0;                                        // one is not prime and also divides everything evenly

    char *ack = (char *) calloc(1, sizeof(char));
    int *base = (int *) calloc(1, sizeof(int));
    *base = 2;
    int *net_info = (int *) calloc(2, sizeof(int));
    net_info[0] = htonl(*max);
    net_info[1] = htonl(*base);
    char *info_buf = (char *)net_info;

    int *removed = (int *) calloc (*max + 1, sizeof(int));
    int *count_removed = (int *) calloc (1, sizeof(int));
    int primes_len = *max - 2;

    // send info to client about data being sent
    if(send(socket_fd, info_buf, sizeof(int) * 2, 0) == -1) {
      perror("server: send()");
      close(socket_fd);
      exit(1);
    }


    if (INTEGRATED) {
      receive_removed(socket_fd, int_list, primes, ack, base, &primes_len);
      calc_loop(socket_fd, removed, primes, ack, base, max, count_removed, &primes_len);
    } else {
      while(*base < *max) {

        if (DEBUG) {
          printf("\n\nBEGIN: Client start of loop\n");
          printf("max = %d\n", *max);
          printf("base = %d\n", *base);
          printf("cnt rmd = %d\n", *count_removed);
          printf("\n\nREMOVED THIS TIME:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *count_removed; i++) {
            printf("%d\t%d\n", i, removed[i]);
          }
          printf("\n\nREMAINING PRIMES:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *max; i++) {
            printf("%d\t%d\n", i, primes[i]);
          }
          printf("\nEND: Client start of loop\n\n");
        } else if (BASE) {
          printf("base = %d\n", *base);
        }

        receive_removed(socket_fd, removed, primes, ack, base, &primes_len);
        printf("\n\nafter receive: primes_len : %d\n", primes_len);
        printf("REMAINING PRIMES:\n-----------------\n");
        for (int i = 0; i < *max; i++) {
          if (primes[i] != 0) {
            printf("%d, ", primes[i]);
          }
        }

        if (DEBUG) {
          printf("\n\nBEGIN: Client between receive and remove mults\n");
          printf("max = %d\n", *max);
          printf("base = %d\n", *base);
          printf("cnt rmd = %d\n", *count_removed);
          printf("\n\nREMOVED THIS TIME:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *count_removed; i++) {
            printf("%d\t%d\n", i, removed[i]);
          }
          printf("\n\nREMAINING PRIMES:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *max; i++) {
            printf("%d\t%d\n", i, primes[i]);
          }
          printf("\nEND: Client between receive and remove mults\n\n");
        } else if (BASE) {
          printf("base = %d\n", *base);
        }

        remove_multiples(primes, base, max, removed, count_removed);
        primes_len -= *count_removed;

        printf("\n\nafter remove_mults: primes_len : %d\n", primes_len);
        printf("REMAINING PRIMES:\n-----------------\n");
        for (int i = 0; i < *max; i++) {
          if (primes[i] != 0) {
            printf("%d, ", primes[i]);
          }
        }

        (*base)++;
        while (*base < *max && primes[*base] == 0) {
          (*base)++;
        }

        if (DEBUG) {
          printf("\n\nBEGIN: Client between remove mults and send\n");
          printf("max = %d\n", *max);
          printf("base = %d\n", *base);
          printf("cnt rmd = %d\n", *count_removed);
          printf("\n\nREMOVED THIS TIME:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *count_removed; i++) {
            printf("%d\t%d\n", i, removed[i]);
          }
          printf("\n\nREMAINING PRIMES:\n-----------------\n");
          printf("Pos\tPrime\n");
          for (int i = 0; i < *max; i++) {
            printf("%d\t%d\n", i, primes[i]);
          }
          printf("\nEND: Client between receive and remove mults\n\n");
        } else if (BASE) {
          printf("base = %d\n", *base);
        }


        if ((send_all(socket_fd, removed, *count_removed, base)) == -1) {
          perror("server: send_all()");
        }
        
        if (DEBUG) {
          printf("\n\nBEGIN: Client between send and receive\n");
          printf("max = %d\n", *max);
          printf("base = %d\n", *base);
          printf("cnt rmd = %d\n", *count_removed);
          printf("\n\nREMOVED THIS TIME:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *count_removed; i++) {
            printf("%d\t%d\n", i, removed[i]);
          }
          printf("\n\nREMAINING PRIMES:\n-----------------\n");
          printf("Pos\tRemoved\n");
          for (int i = 0; i < *max; i++) {
            printf("%d\t%d\n", i, primes[i]);
          }
          printf("\nEND: Client between receive and remove mults\n\n");
        } else if (BASE) {
          printf("base = %d\n", *base);
        }
      }

      char type[] = "client";
      printf("DONEDONEODNE");
      printf("primes_len: %d\n", primes_len);
      printOut(type, primes, *max, 5);
      /*
      printf("\n\nREMAINING PRIMES:\n-----------------\n");
      printf("Pos\tPrime\n");
      for (int i = 0; i < *max; i++) {
        if (primes[i] != 0) {
          printf("%d\t%d\n", i, primes[i]);
        }
      }
      */
    }


    return 0;

    // -------------------- CLIENT END -------------------- //
  }
}




// ----------- ORPHANED CODE ----------- //
    /*
      receive_removed(socket_fd, int_list, primes, ack, base);
    while(*base < max) {
      remove_multiples(primes, base, &max, removed, count_removed);


      send_all(socket_fd, removed, *count_removed, base);
      receive_removed(socket_fd, int_list, primes, ack, base);
    }


    printf("Pos\tListItem");
    for (int i = 0; i < max; i++) {
      printf("%d\t%d\n", i, primes[i]);
    }

    // wait for a listening acknowledgement from server
    while (ack[0] != ACK) {
      if (recv(socket_fd, ack, sizeof(char), 0) == -1) {
          perror("client: recv() ACK");
          exit(1);
      }
    }
    */
