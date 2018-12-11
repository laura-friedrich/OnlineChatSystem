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

void *client_func(void *data);
int client_pids[MAX_CLIENTS];

int main(int argc, char *argv[])
{
    // Allocate memory for client_pids
    //client_pids = (int **) malloc (MAX_CLIENTS * sizeof (int));
    for(int i = 0; i < MAX_CLIENTS; i++){
      client_pids[i] = -1; // Initialize all elements to -1 to start
      //printf("Client PID %d:%d.\n", i, client_pids[i]);
    }
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;
    char *remote_ip;
    char buf[BUF_SIZE];
    int bytes_received;

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
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

        /* announce our communication partner */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        // New client thread
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_func, NULL);

        /* receive and echo data until the other end closes the connection */
        while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {
            if(bytes_received == -1){
              perror("recv error");
            }
            // for(int i = 0; i < BUF_SIZE; i++){
            //   printf("%d", buf[i]);
            // }
            //printf("Buf: %s", buf);
            // Change username
            fflush(stdout);

            /* send it back */
            if(send(conn_fd, buf, bytes_received, 0) == -1){
              perror("Send error");
            }
        }
        printf("\n");

        close(conn_fd);
    }
}


void* client_func(void *data){
    for(int i = 0; i < MAX_CLIENTS; i++){
      if(client_pids[i] == -1){
        pid_t pid;
        pid = getpid();
        printf("I am the child. I am client %d. My pid is %d.\n", i, pid);
        client_pids[i] = pid;
        break; // Break once the pid has been assigned to array.
      }
      //printf("Client PID %d:%ls.\n", i, client_pids[i]);
    }


    return NULL;
}
