#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_ROOMS 1000000
#define MAX_CLIENTS_PER_ROOM 2

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

void *accept_clients(void *arg);

extern Room rooms[MAX_ROOMS];

#endif
