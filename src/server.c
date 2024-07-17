#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"

#define MAX_BUFFER_SIZE 5

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

		if((strcmp(buffer, "P") == 0 && client->color == false) || (strcmp(buffer, "p") == 0 && client->color == true)) {
			continue;
		}

		pthread_mutex_lock(&(rooms[client->room_id].mutex));
		for (int i = 0; i < rooms[client->room_id].num_clients; i++) {
			int target_socket = rooms[client->room_id].clients[i].socket;
			if (target_socket == client_socket) {
				continue;
			}
			if (send(target_socket, buffer, strlen(buffer), 0) == -1) {
				perror("Send failed");
				break;
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

			rooms[room_id].clients[rooms[room_id].num_clients].color = rooms[room_id].num_clients == 0 ? rand() % 2 == 0 : !rooms[room_id].clients[0].color;

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
