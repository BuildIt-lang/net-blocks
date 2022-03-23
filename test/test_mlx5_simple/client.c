#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#define CLIENT_MSG ("Hello from client")

char client_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x36, 0x33};
char server_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x2e, 0xeb};

int logs[200] = {0};

unsigned long long last = -1;
int total = 0;

unsigned long long get_time_in_us(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}


int running = 1;
static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_READ_READY) {
		char buff[65];
		int len = nb__read(c, buff, 64);
		unsigned long long now = get_time_in_us();
		total++;
		unsigned long long diff = now - last;
		if (diff < 200) 
			logs[diff]++;	

		if (total > 10000)
			running = 0;
		//printf("Received in %d\n", (int) diff);
		last = get_time_in_us();
		
		nb__send(c, CLIENT_MSG, sizeof(CLIENT_MSG));
	}
}

int main(int argc, char* argv[]) {
	nb__mlx5_init();
	memcpy(nb__my_host_id, client_id, 6);

	nb__connection_t * conn = nb__establish(server_id, 8080, 8081, callback);

	last = get_time_in_us();	
	nb__send(conn, CLIENT_MSG, sizeof(CLIENT_MSG));

	while (running) {
		nb__main_loop_step();
	}
	nb__destablish(conn);

	for (int i = 0; i < 30; i++)
		printf("%d: %d\n", i, logs[i]);
	return 0;
	
}
