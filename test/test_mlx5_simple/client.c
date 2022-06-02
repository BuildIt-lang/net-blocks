#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

char msg[64] = {'x'};
char client_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x36, 0x33};
char server_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x2e, 0xeb};

int logs[200] = {0};

unsigned long long last = -1;
int total = 0;

unsigned long long get_time_in_ns(void) {
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec * 1000000000ull + tv.tv_nsec;
}


int running = 1;
static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_READ_READY) {
		char buff[65];
		int len = nb__read(c, buff, 64);
		unsigned long long now = get_time_in_ns();
		total++;
		unsigned long long diff = (now - last)/100;
		if (diff < 200) 
			logs[diff]++;	

		if (total > 100000)
			running = 0;
		last = get_time_in_ns();
		nb__send(c, msg, sizeof(msg));	
	}
}

extern int nb__mlx5_simulate_packet_drop;
int main(int argc, char* argv[]) {
	srand(time(NULL));
	//nb__mlx5_simulate_packet_drop = 1;
	nb__mlx5_init();
	nb__net_init();
	memcpy(nb__my_host_id, client_id, 6);

	nb__connection_t * conn = nb__establish(server_id, 8080, 8081, callback);

	last = get_time_in_ns();	
	unsigned long long start = last;
	
	nb__send(conn, msg, sizeof(msg));

	while (running) {
		nb__main_loop_step();
	}

	unsigned long long end = get_time_in_ns();
	printf("Time elapsed = %llu us\n", (end - start) / 1000);
	nb__destablish(conn);
	
	for (int i = 0; i < 60; i++)
		printf("%d\n", logs[i]);
	return 0;
	
}
