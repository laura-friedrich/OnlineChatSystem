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


typedef struct ClientStruct{
    int conn_fd;
    char *name;
    char *remote_ip;
    int remote_port;
}ClientStruct;

void *client_func(void *data);
char buf[BUF_SIZE];
int bytes_received;

int main(int argc, char *argv[])
{
  // Allocate memory for client_pids
  char *listen_port;
  struct addrinfo hints, *res;
  int rc;
  struct sockaddr_in remote_sa;
  uint16_t remote_port;
  socklen_t addrlen;
  char *remote_ip;
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
    struct ClientStruct *data_for_thread = malloc(sizeof(struct ClientStruct));

    /* accept a new connection (will block until one appears) */
    addrlen = sizeof(remote_sa);
    int conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

    /* announce our communication partner */
    remote_ip = inet_ntoa(remote_sa.sin_addr);
    remote_port = ntohs(remote_sa.sin_port);

    data_for_thread->conn_fd = conn_fd;
    data_for_thread->remote_ip = remote_ip;
    data_for_thread->remote_port = remote_port;
    data_for_thread->name = "Unknown";

    printf("new connection from %s:%d\n", data_for_thread->remote_ip, data_for_thread->remote_port);

    // New client thread
    pthread_t client_thread;
    int ret = pthread_create(&client_thread, NULL, client_func, data_for_thread);
     if (ret) {
      printf("ERROR: Return Code from pthread_create() is %d\n", ret);
      exit(1);
     }
    pthread_join(client_thread, NULL);

    printf("\n");

    close(conn_fd);
  }
}


void* client_func(void *data){
  struct ClientStruct *client_data = data;

  /* receive and echo data until the other end closes the connection */
  while((bytes_received = recv(client_data->conn_fd, buf, BUF_SIZE, 0)) > 0) {
      if(bytes_received == -1){
        perror("recv error");
      }

      // Change username
      fflush(stdout);

      /* send it back */
      if(send(client_data->conn_fd, buf, bytes_received, 0) == -1){
        perror("Send error");
      }
      read(0, buf, 1);
    }

  return NULL;
}
