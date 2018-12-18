/* 	Server #2 - CS1652 Jack Lange */

#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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

#define MAX_CONNECTIONS 10
#define BUFSIZE 1024

int main(int argc, char *argv[]) {


	/*-----------*/
	/* Variables */
	struct sockaddr_in server;
	int len = sizeof(server);
	char in_buffer[BUFSIZE];
	char out_buffer[BUFSIZE];
	int server_port = -1;
	int listening_sock = 0;
	int connection_sock;
	int connections[MAX_CONNECTIONS];
	fd_set readfds;	// connections ready to be read
	int i, j;

	/* Initialize connections[] to all 0's, no connections exist */
	for(i = 0; i < MAX_CONNECTIONS; i++) {
		connections[i] = 0;
	}


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
		listening_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(listening_sock < 0) {
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
	if(bind(listening_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		fprintf(stderr, "Bind error. Terminating...\n");
		exit(0);
	}


	/*--------------------------------------------*/
	/* Listen: "create a client connection queue" */
	if(listen(listening_sock, 32) < 0) {	// Potential error when backlog is set to 0
		fprintf(stderr, "Listen error. Terminating...\n");
		exit(0);
	}


	/*------------------------------------------------------*/
	/* Accept and handle connections.  DIFFERS FROM SERVER1 */
	printf("Server running on port %d. Waiting for connections...\n", server_port); 
	while(1) {
		
		/* Reset the fd_set readfds */
		FD_ZERO(&readfds);

		/* Add the listening socket to the readfds */
		FD_SET(listening_sock, &readfds);
		int highest_sock = listening_sock, curr_sock; // Used for iteration
		
		/* Reinitialize connections[] by looping, if connection exists, FD_SET it */
		/* Potentially update the file descriptor for the highest socket */
		for(i = 0; i < MAX_CONNECTIONS; i++) {
			curr_sock = connections[i];

			if(curr_sock > 0) FD_SET(curr_sock, &readfds);
			if(curr_sock > highest_sock) highest_sock = curr_sock;
		}

		/* SELECT - Wait for something to happen at one of the sockets, proceed accordingly */
		int nfds = highest_sock + 1;
		int select_wait = select(nfds, &readfds, NULL, NULL, NULL); 

		/* If the listening_sock is readable, then a new connection is happening */
		if(FD_ISSET(listening_sock, &readfds)) {

			connection_sock = accept(listening_sock, (struct sockaddr *)&server, &len);
			if(connection_sock < 0) {
				printf("Connection fd: %d\n", connection_sock);
				fprintf(stderr, "Error accepting connection. Terminating..\n");
				exit(1);
			} 

			printf("A connection was established.\n");

			// Add new connection_sock to the first available array slot
			for(i = 0; i < MAX_CONNECTIONS; i++) {
				if(connections[i] == 0) {
					connections[i] = connection_sock;
					break;
				} 
			}
		}


		/* Regardless, loop through and find a socket that is readable	*/
		for(i = 0; i < MAX_CONNECTIONS; i++) {
			connection_sock = connections[i];

			/* If the socket here is readable, then perform message handling */
			/* actions from http_server1 (copied directly from last part) */
			if(FD_ISSET(connection_sock, &readfds)) {
				memset(in_buffer, '\0', BUFSIZE);
				read(connection_sock, in_buffer, sizeof(in_buffer) - 1);

				j = 0;
				char *str = strtok(in_buffer, " "), *path, *hostname;
				while(str != NULL){
					str = strtok(NULL, " ");

					if(j == 0) {
						path = str;
					} else if(j == 2) {
						hostname = str;
					}

					j++;
				}
			
				/* Attempt to open the file requested */
				FILE *file;
				file = fopen(path, "r");
				
				/* If it couldn't be found */
				if(file == NULL) {
					fprintf(stderr, "The file requested could not be found. 404 sent.\n");
					sprintf(out_buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
					write(connection_sock, out_buffer, strlen(out_buffer));
				
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
					
					write(connection_sock, out_buffer, strlen(out_buffer));
					write(connection_sock, file_buffer, file_size);
					printf("A response was sent.\n");
				}

				/* Close the connection update some things */
				close(connection_sock);
				connections[i] = 0;
				FD_CLR(connection_sock, &readfds);
				printf("Connection closed.\n");
			}
		}

		



		/*
		/*
		connection_sock = accept(listening_sock, (struct sockaddr *)&server, &len);
		if(connection_sock < 0) {
			fprintf(stderr, "Error accepting connection. Terminating...\n");
			exit(0);
		}

		printf("\nConnection received!\n"); //DEBUGGING
		

		/* Parse request, attempt to open file at path, return error if bad request 
		/* Loop through message with strtok, store names sent 
		memset(in_buffer, '\0', BUFSIZE);
		read(connection_sock, in_buffer, sizeof(in_buffer) - 1);
		
		i = 0;
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

		
		/* If the file couldn't be found 
		FILE *file;
		file = fopen(path, "r");
		if(file == NULL) {
			fprintf(stderr, "The file requested could not be found. Returning error message.\n");
			sprintf(out_buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
			write(connection_sock, out_buffer, strlen(out_buffer));
		
		/* Write response to client otherwise*/
		/* First read from file to buffer, then write response and file to client 
		} else {

			int file_size;
			fseek(file, 0, SEEK_END);
			file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			char file_buffer[file_size];
			fread(file_buffer, file_size, 1, file);
			sprintf(out_buffer, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", file_size);
			
			write(connection_sock, out_buffer, strlen(out_buffer));
			write(connection_sock, file_buffer, file_size);
			printf("Response sent!\n");
		}


		/* Close connection_sock 
		close(connection_sock);
		printf("Connection closed!\n");
		*/
	}

	
	/*--------------*/
	/* Close socket */
	close(listening_sock);
}
