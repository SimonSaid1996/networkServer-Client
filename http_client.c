/* HTTP Clinet - CS1652 Jack Lange */

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

int main(int argc, char * argv[]) {

  /* Variables */
  int s, port, len, res;
  char *hostname = argv[2];
  char *path = argv[4];
  char send_buffer[BUFSIZE];
  char recv_buffer[BUFSIZE];
  struct hostent *hp;
  struct sockaddr_in saddr;	

  /* Parse command line arguments */
  if(argc != 5 || (port = atoi(argv[3])) <= 1500 || port > 65535) {	
    fprintf(stderr, "usage: http_client <k|u> <server> <port> <path> (port must be > 1500)\n");
    exit(-1);
  }

  /* Create socket */
  s = 0;
  if((s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0 ){
    fprintf(stderr,"Error encountered creating socket.");	//print out some sort of error
    exit(-1);
  }


  /* Convert hostname to IP address */
  if((hp = gethostbyname( argv[2]) ) == NULL){ 
    fprintf(stderr,"Invalid server entered.\n");
    exit(-1);
  }

  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  memcpy(&saddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
  saddr.sin_port = htons(port);


  /* Establish connection to server */
  if(connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){
    fprintf(stderr,"Couldn't connect to the server.");
    exit(-1);
  }


  /* Assemble GET request to be sent to server NEEDS WORK */
  memset(send_buffer, '\0', BUFSIZE);
  sprintf(send_buffer + strlen(send_buffer), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);
  
  len = strlen(send_buffer);
  printf("Message Being Sent:\n%s", send_buffer);


  /* Attempt to write to the socket */
  if((res = write(s, send_buffer, len)) <= 0){
    fprintf(stderr, "Error encountered sending message to the server.");
    exit(-1);
  }


  /* Check and print response from server */
  memset(recv_buffer, '\0', BUFSIZE);
  if ((res = read(s, recv_buffer, sizeof(recv_buffer) -1 )) <= 0){
    fprintf(stderr,"No response received from the server.");
    exit(-1);
  }
  printf("Response Received:\n%s\n", recv_buffer);


  /* Close socket */
  close(s);
  return 0;
}