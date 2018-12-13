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

#define BACKLOG 10
#define BUF_SIZE 4096
#define MAX_CLIENTS 256
#define DEFAULT_CLIENT_COUNT 10

typedef struct ClientStruct{
    int conn_fd;
    char *name;
    char *remote_ip;
    int remote_port;
}ClientStruct;

void *client_func(void *data);
struct ClientStruct *clients[DEFAULT_CLIENT_COUNT];
int clientCounter = 0;

int main(int argc, char *argv[])
{
  // Allocate memory for client_pids
  char *listen_port;
  struct addrinfo hints, *res;
  int rc;
  struct sockaddr_in remote_sa;
  int listen_fd;

  listen_port = argv[1];

  /* create a socket */
  listen_fd = socket(PF_INET, SOCK_STREAM, 0);

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
  }

  /* start listening */
  if(listen(listen_fd, BACKLOG) == -1){
    perror("Listen error");
  }

  /* infinite loop of accepting new connections and handling them */
  while(1) {
    clients[clientCounter] = malloc(sizeof(struct ClientStruct));
    /* accept a new connection (will block until one appears) */
    socklen_t addrlen = sizeof(remote_sa);
    int conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

    /* announce our communication partner */
    char *remote_ip = inet_ntoa(remote_sa.sin_addr);
    uint16_t remote_port = ntohs(remote_sa.sin_port);

    clients[clientCounter]->conn_fd = conn_fd;
    clients[clientCounter]->remote_ip = remote_ip;
    clients[clientCounter]->remote_port = remote_port;
    clients[clientCounter]->name = "Unknown";

    printf("new connection from %s:%d\n", clients[clientCounter]->remote_ip, clients[clientCounter]->remote_port);

    // New client thread
    pthread_t client_thread;
    int ret = pthread_create(&client_thread, NULL, client_func, clients[clientCounter]);
     if (ret) {
      printf("ERROR: Return Code from pthread_create() is %d\n", ret);
      exit(1);
     }
     clientCounter++;
    //printf("\n");
    //pthread_join(client_thread, NULL);
    //close(data_for_thread->conn_fd);
  }
}


void* client_func(void *data){
  int bytes_received = 0;
  char buf[BUF_SIZE];
  struct ClientStruct *client_data = data;

  /* receive and echo data until the other end closes the connection */
  while((bytes_received = recv(client_data->conn_fd, buf, BUF_SIZE, 0)) > 0) {
      if(bytes_received == -1){
        perror("recv error");
      }

      printf("Bytes recieved %d: ", bytes_received);
      if(strcmp(buf, "/nick")){}
      // Change username
      fflush(stdout);

      /* send it back */
      if(send(client_data->conn_fd, buf, bytes_received, 0) == -1){
        perror("Send error");
      }
      // Send message to all clients
      for(int i = 0; i < clientCounter; i++){
         printf("Trying to send message to client %d, conn_fd %d.\n", i, clients[i]->conn_fd);
         if(send(clients[i]->conn_fd, "MESSAGE", 8, 0) == -1){
           perror("Error sending to all clients.");
         }
       }

      //read(0, buf, 1);
    }

  return NULL;
}
