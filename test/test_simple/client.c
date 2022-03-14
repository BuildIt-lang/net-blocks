#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>
#define CLIENT_MSG ("Hello from client")
int running = 1;
static void callback(int event, nbr__connection_t * c) {
	if (event == QUEUE_EVENT_READ_READY) {
		char buff[65];
		int len = nb__read(c, buff, 64);
		buff[len] = 0;	
		printf("Received = %s\n", buff);	
		running = 0;
	}
}

int main(int argc, char* argv[]) {
	nb__ipc_init("/tmp/ipc_socket", 0);
	printf("IPC initialized\n");
	nbr__my_host_id = 10002;

	nbr__connection_t * conn = nb__establish(10001, 8080, 8081, callback);
	
	nb__send(conn, CLIENT_MSG, sizeof(CLIENT_MSG));

	while (running) {
		nbr__main_loop_step();
		usleep(100 * 1000);
	}
	nb__destablish(conn);
	return 0;
	
}
