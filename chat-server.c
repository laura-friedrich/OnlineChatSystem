/*
* echo-server.c
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define BACKLOG 10
#define BUF_SIZE 4096
#define MAX_NUMBER_OF_ARGS 100

pthread_mutex_t mutex;

typedef struct ClientStruct{
  int conn_fd;
  char *name;
  char *remote_ip;
  int remote_port;
  pthread_t thread;
  pid_t pid;
  int clientNumber;
}ClientStruct;

int initial_client_count = 2;
void *client_func(void *data);
struct ClientStruct **clients;
int clientCounter = 0;
int main(int argc, char *argv[])
{
  // Allocate memory for clients
  clients = malloc(initial_client_count * sizeof(ClientStruct));

  // Allocate memory for client_pids
  char *listen_port;
  struct addrinfo hints, *res;
  int rc;
  struct sockaddr_in remote_sa;
  int listen_fd;
  listen_port = argv[1];

  /* create a socket */
  listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  if(listen_fd == -1){
    perror("Error creating listen socket.");
  }

  /* bind it to a port */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
    printf("getaddrinfo failed: %s\n", gai_strerror(rc));
    exit(1);
  }

  if (bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1){
    perror("Bind error.");
    exit(1);
  }

  /* start listening */
  if(listen(listen_fd, BACKLOG) == -1){
    perror("Listen error");
    exit(1);
  }

  /* infinite loop of accepting new connections and handling them */
  while(1) {//
    if(clientCounter > initial_client_count){ // Dynamically allocate more memory for new clients
      initial_client_count = initial_client_count * 2;
      clients = realloc(clients, initial_client_count * sizeof(ClientStruct));
    }else if(clientCounter < initial_client_count * 3 && clientCounter != 0){ // Dynamically allocate less memory if clients have left
      initial_client_count = initial_client_count / 2;
      clients = realloc(clients, initial_client_count * sizeof(ClientStruct));
    }
    clients[clientCounter] = malloc(sizeof(struct ClientStruct));
    /* accept a new connection (will block until one appears) */
    socklen_t addrlen = sizeof(remote_sa);
    int conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);
    if(conn_fd == -1){
      perror("Error accepting connection.");
      exit(0);
    }

    /* announce our communication partner */
    char *remote_ip = inet_ntoa(remote_sa.sin_addr);
    uint16_t remote_port = ntohs(remote_sa.sin_port);

    pthread_t client_thread;

    clients[clientCounter]->conn_fd = conn_fd;
    clients[clientCounter]->remote_ip = remote_ip;
    clients[clientCounter]->remote_port = remote_port;
    clients[clientCounter]->name = "Unknown";
    clients[clientCounter]->thread = client_thread;
    clients[clientCounter]->clientNumber = clientCounter;

    printf("new connection from %s:%d\n", clients[clientCounter]->remote_ip, clients[clientCounter]->remote_port);

    // New client thread
    int ret = pthread_create(&client_thread, NULL, client_func, clients[clientCounter]);
    if (ret) {
      printf("ERROR: Return Code from pthread_create() is %d\n", ret);
      exit(1);
    }
    clientCounter++;
  }
}


void* client_func(void *data){
  int bytes_received = 0;
  char buf[BUF_SIZE];
  struct ClientStruct *client_data = data;
  pid_t clientPID;
  clientPID = getpid();
  clients[client_data->clientNumber]->pid = clientPID; // Assigning pid to process
  /* receive and echo data until the other end closes the connection */

  while((bytes_received = recv(client_data->conn_fd, buf, BUF_SIZE, 0)) > -1) {
    pthread_mutex_lock(&mutex);
    buf[bytes_received] = '\0';// Make last byte the null byte
    if(bytes_received == 0){
      //free(clients[client_data->clientNumber]);

      char sendBuf [512];
      sprintf(sendBuf,"User %s (%s:%d) has disconnected", client_data->name, client_data->remote_ip, client_data->remote_port);
      for(int i = 0; i < clientCounter; i++){
        if(i != client_data->clientNumber){
          if(send(clients[i]->conn_fd, sendBuf, sizeof(buf), 0) == -1){
            perror("Error sending to all clients.");
          }
        }
      }

      for (int c = client_data->clientNumber; c < initial_client_count; c++){ // Reassign other indices of clients
         clients[c + 1]->clientNumber = c;
         clients[c] = clients[c+1];
       }
       free(client_data); // Free struct
       clientCounter--;

      //kill(clientPID, 0);
      puts(sendBuf);
      fflush(stdout);
    }else if(strncmp(buf, "/nick ", 6)==0){
      char nickName[bytes_received];
      memcpy(nickName,(char*)buf+6,bytes_received-6);
      nickName[bytes_received-7] = '\0';// Make last byte the null byte
      char sendBuf [512];
      sprintf(sendBuf,"User %s (%s:%d) is now known as %s", client_data->name, client_data->remote_ip, client_data->remote_port, nickName);
      puts(sendBuf);
      client_data->name = malloc(sizeof(nickName) * 2);
      strcpy(client_data->name , nickName);
      //printf()
      for(int i = 0; i < clientCounter; i++){
        if(send(clients[i]->conn_fd,sendBuf , sizeof(buf), 0) == -1){
          perror("Error sending to all clients.");
        }
      }
      fflush(stdout);

    }else{ // Send mesage back with name
      for(int i = 0; i < clientCounter; i++){
        char sendBuf [8192];
        sprintf(sendBuf, "%s: %s", client_data->name, buf);
        if(send(clients[i]->conn_fd, sendBuf, sizeof(buf), 0) == -1){
          perror("Error sending to all clients.");
        }
      }
      fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}
