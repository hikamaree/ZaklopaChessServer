#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ROOMS 1000
#define MAX_CLIENTS_PER_ROOM 2
#define MAX_BUFFER_SIZE 5

typedef struct {
	int socket;
	pthread_t thread_id;
	int room_id;
	bool color;
} Client;

typedef struct {
	Client clients[MAX_CLIENTS_PER_ROOM];
	int num_clients;
	pthread_mutex_t mutex;
} Room;

Room rooms[MAX_ROOMS];

void *handle_client(void *arg) {
	Client *client = (Client*)arg;
	int client_socket = client->socket;
	char buffer[MAX_BUFFER_SIZE];

	if (send(client_socket, &client->color, sizeof(client->color), 0) == -1) {
		perror("Send failed");
		close(client_socket);
		pthread_exit(NULL);
	}

	if (rooms[client->room_id].num_clients == MAX_CLIENTS_PER_ROOM) {
		for(int i = 0; i < MAX_CLIENTS_PER_ROOM; i++) {
			send(rooms[client->room_id].clients[i].socket, "play", strlen("play"), 0);
		}
	}

	while(1) {
		memset(buffer, 0, sizeof(buffer));

		ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
		if (bytes_received == -1) {
			perror("Receive failed");
			break;
		} else if (bytes_received == 0) {
			printf("Client disconnected\n");
			break;
		}

		pthread_mutex_lock(&(rooms[client->room_id].mutex));
		for (int i = 0; i < rooms[client->room_id].num_clients; i++) {
			int target_socket = rooms[client->room_id].clients[i].socket;
			if (target_socket != client_socket) {
				if (send(target_socket, buffer, strlen(buffer), 0) == -1) {
					perror("Send failed");
					break;
				}
			}
		}
		pthread_mutex_unlock(&(rooms[client->room_id].mutex));

		if (strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "P") == 0 || strcmp(buffer, "p") == 0) {
			printf("Game in room %d ended\n", client->room_id);
			break;
		}
	}

	pthread_mutex_lock(&(rooms[client->room_id].mutex));
	for (int i = 0; i < rooms[client->room_id].num_clients; i++) {
		if (rooms[client->room_id].clients[i].socket != client_socket) {
			close(rooms[client->room_id].clients[i].socket);
		}
	}
	rooms[client->room_id].num_clients = 0;
	pthread_mutex_unlock(&(rooms[client->room_id].mutex));

	close(client_socket);
	pthread_exit(NULL);
}

void *accept_clients(void *arg) {
	int server_socket = *((int*)arg);
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (1) {
		int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client_socket == -1) {
			perror("Accept failed");
			continue;
		}

		printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		char room_id_str[10];
		ssize_t bytes_received = recv(client_socket, room_id_str, sizeof(room_id_str), 0);
		if (bytes_received <= 0) {
			perror("Receive failed");
			close(client_socket);
			continue;
		}
		room_id_str[bytes_received] = '\0';

		int room_id = atoi(room_id_str);
		if (room_id < 0 || room_id >= MAX_ROOMS) {
			printf("Invalid room ID received\n");
			close(client_socket);
			continue;
		}

		pthread_mutex_lock(&(rooms[room_id].mutex));
		if (rooms[room_id].num_clients < MAX_CLIENTS_PER_ROOM) {
			rooms[room_id].clients[rooms[room_id].num_clients].socket = client_socket;
			rooms[room_id].clients[rooms[room_id].num_clients].room_id = room_id;

			if (rooms[room_id].num_clients == 0) {
				rooms[room_id].clients[rooms[room_id].num_clients].color = rand() % 2 == 0;
			} else {
				rooms[room_id].clients[rooms[room_id].num_clients].color = !rooms[room_id].clients[0].color;
			}

			rooms[room_id].num_clients++;
			pthread_create(&(rooms[room_id].clients[rooms[room_id].num_clients - 1].thread_id), NULL, handle_client, &(rooms[room_id].clients[rooms[room_id].num_clients - 1]));
			pthread_mutex_unlock(&(rooms[room_id].mutex));
		} else {
			printf("Game room %d is full. Connection rejected.\n", room_id);
			pthread_mutex_unlock(&(rooms[room_id].mutex));
			close(client_socket);
		}
	}
}

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
