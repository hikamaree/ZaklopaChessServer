#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "server.h"

int main(int argc, char* argv[]) {
	srand(time(NULL));

	for (int i = 0; i < MAX_ROOMS; i++) {
		rooms[i].num_clients = 0;
		pthread_mutex_init(&(rooms[i].mutex), NULL);
	}

	if (argc < 2) {
		printf("Missing port\n");
		return 1;
	}

	int port;
	if (sscanf(argv[1], "%d", &port) != 1) {
		printf("Invalid port\n");
		return 1;
	}

	int server_socket;
	struct sockaddr_in server_addr;
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		return 1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("Binding failed");
		return 1;
	}

	if (listen(server_socket, 10) == -1) {
		perror("Listening failed");
		return 1;
	}

	printf("Server listening on port %d...\n", port);

	pthread_t tid;
	if (pthread_create(&tid, NULL, accept_clients, &server_socket) != 0) {
		perror("Thread creation failed");
		return 1;
	}

	pthread_join(tid, NULL);

	for (int i = 0; i < MAX_ROOMS; i++) {
		pthread_mutex_destroy(&(rooms[i].mutex));
	}

	close(server_socket);
	return 0;
}
