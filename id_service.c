#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>

#include "conf.h"
#include "id_service.h"

static int listen_sd;

void *thr_worker(void *p)
{
	
}

void id_service_start(void)
{
	int err;
	int client_sd;
	struct sockaddr_in local_addr, client_addr;
	socklen_t client_addr_size;
	pthread_t tid;

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sd<0) {
		perror("socket()");
		exit(1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(DEFAULT_MONITOR_PORT);
	local_addr.sin_addr.s_addr = 0;
	if (bind(listen_sd, (void*)&local_addr, sizeof(local_addr))<0) {
		perror("bind()");
		exit(1);
	}

	if (listen(listen_sd, 512)<0) {
		perror("listen");
		exit(0);
	}

	client_addr_size = sizeof(client_addr);
	while (1) {
		client_sd = accept(listen_sd, (void*)&client_addr, &client_addr_size);
		err = pthread_create(&tid, NULL, thr_worker, (void*)client_sd);
		if (err) {
			syslog(LOG_WARNING, "pthread_create() failed: %s", strerror(err));
			close(client_sd);
		}
	}

	close(listen_sd);
}

