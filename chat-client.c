/*
 * echo-client.c
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE 4096

int conn_fd;
void* listenForCommands(void *data);
void* writeCommands(void *data);

int main(int argc, char *argv[])
{
    printf("Starting chat-client");
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    //int n;
    int rc;

    dest_hostname = argv[1];
    dest_port     = argv[2];

    /* create a socket */
    conn_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* client usually doesn't bind, which lets kernel pick a port number */

    /* but we do need to find the IP address of the server */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    /* connect to the server */
    if(connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        exit(2);
    }

    printf("\nConnected\n");

    /* infinite loop of reading from terminal, sending the data, and printing
     * what we get back */

    pthread_t listen_thread, write_thread;
    int ret;
    if((ret = pthread_create(&listen_thread, NULL, listenForCommands, NULL))){
      printf("ERROR: Return Code from pthread_create() is %d\n", ret);
    }

    if((ret = pthread_create(&write_thread, NULL, writeCommands, NULL))){
      printf("ERROR: Return Code from pthread_create() is %d\n", ret);
    }

    pthread_join(write_thread, NULL);
    pthread_join(listen_thread, NULL);

    fflush(stdout);


    //close(conn_fd);
}


void* writeCommands(void *data){
char buf[BUF_SIZE];
int n;
  while((n = read(0, buf, BUF_SIZE)) > 0){
    //puts(buf);
    if(send(conn_fd, buf, n, 0) == -1){
      perror("Failed to send.");
    }
      // n = recv(conn_fd, buf, BUF_SIZE, 0);
      // if(strcmp(buf, "000") == 0){
      //   printf("\nConnected\n");
      // }else{
      //   puts(buf);
      // }
    }
    //printf("writeCommands returning");
  return NULL;
}

void* listenForCommands(void *data){
  char buf[BUF_SIZE];
  int n;
  while(0 == 0){
    n = recv(conn_fd, buf, BUF_SIZE, 0);
    if(n > 0){
      //printf("N is greater than 0 (%d).\n", n);
      puts(buf);
    }
    //fflush(stdout);
  }

  //printf("listenForCommands returning");
  return NULL;
}
