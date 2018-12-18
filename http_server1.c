/* 	Server #1 - CS1652 Jack Lange */

#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 1024

int main(int argc, char *argv[]) {


	/*-----------*/
	/* Variables */
	struct sockaddr_in server;
	int len = sizeof(server);
	char in_buffer[BUFSIZE];
	char out_buffer[BUFSIZE];
	
	int server_port = -1;
	int sock 		=  0;
	int connection;
	//int rc		= -1;

	
	/*-------------------------------------------------------------*/	
	/* Check command line args, print error if format is incorrect */
	if(argc != 3) {
		fprintf(stderr, "Usage: http_server1 <k|u> <port>\n");
		exit(0);
	}

	
	/*-------------------------------------------*/
	/* Set server_port value, check its validity */
	server_port = atoi(argv[2]);

	if(server_port < 1500) {
		fprintf(stderr, "Invalid port! (%d) Number cannot be less than 1500\n", server_port);
		exit(0);
	}

	
	/*-------------------------------------------------------------------------*/
	/* Initialize and set up listening socket. MINET NOT AVAILABLE FOR USE YET */
	if(toupper(*(argv[1])) == 'K') {
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sock < 0) {
			fprintf(stderr, "Error encountered creating socket.  Terminating...\n");
			exit(0);
		}
	
	} else if(toupper(*(argv[1])) == 'U') {
		fprintf(stderr, "Minet not yet available for use! Terminating...\n");
		exit(0);
	
	} else {
		fprintf(stderr, "Invalid argument! Must use either \"k\" or \"u\". Terminating...\n");
		exit(0);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = INADDR_ANY;


	/*----------------------------------------*/
	/* Bind: "reserve the port on interfaces" */
	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		fprintf(stderr, "Bind error. Terminating...\n");
		exit(0);
	}


	/*--------------------------------------------*/
	/* Listen: "create a client connection queue" */
	if(listen(sock, 32) < 0) {
		fprintf(stderr, "Listen error. Terminating...\n");
		exit(0);
	}


	/*-------------------------------*/
	/* Accept and handle connections */
	printf("Server running on port %d. Waiting for connections...\n", server_port); //DEBUGGING
	while(1) {
		
		connection = accept(sock, (struct sockaddr *)&server, &len);
		if(connection < 0) {
			fprintf(stderr, "Error accepting connection. Terminating...\n");
			exit(0);
		}

		printf("\nConnection received!\n"); //DEBUGGING

		/* Parse request, attempt to open file at path, return error if bad request */
		/* Loop through message with strtok, store names sent */
		memset(in_buffer, '\0', BUFSIZE);
		read(connection, in_buffer, sizeof(in_buffer) - 1);
		
		int i = 0;
		char *str = strtok(in_buffer, " "), *path, *hostname;
		while(str != NULL){
			str = strtok(NULL, " ");

			if(i == 0) {
				path = str;
			} else if(i == 2) {
				hostname = str;
			}

			i++;
		}

		
		/* If the file couldn't be found */
		FILE *file;
		file = fopen(path, "r");
		if(file == NULL) {
			fprintf(stderr, "The file requested could not be found. Returning error message.\n");
			sprintf(out_buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
			write(connection, out_buffer, strlen(out_buffer));
		
		/* Write response to client otherwise*/
		/* First read from file to buffer, then write response and file to client */
		} else {

			int file_size;
			fseek(file, 0, SEEK_END);
			file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			char file_buffer[file_size];
			fread(file_buffer, file_size, 1, file);
			sprintf(out_buffer, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", file_size);
			
			write(connection, out_buffer, strlen(out_buffer));
			write(connection, file_buffer, file_size);
			printf("Response sent!\n");
		}


		/* Close connection */
		close(connection);
		printf("Connection closed!\n");
	}

	
	/*--------------*/
	/* Close socket */
	close(sock);
}
