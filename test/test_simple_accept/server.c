#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>

#define SERVER_MSG ("Hello from server")

char client_id[] = "1.0.0.2";
char server_id[] = "1.0.0.1";

int running = 1;
static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_ACCEPT_READY) {
		// Share the callback function
		nb__connection_t * s = nb__accept(c, callback);
	} else if (event == QUEUE_EVENT_READ_READY) {	
		char buff[65];
		int len = nb__read(c, buff, 64);
		buff[len] = 0;	
		printf("Received = %s\n", buff);	
		nb__send(c, SERVER_MSG, sizeof(SERVER_MSG));
		running = 0;
	} else if (event == QUEUE_EVENT_ESTABLISHED) {
		printf("Signaling state = %d\n", c->signaling_state);
		printf("Ready to transmit %p\n", c);
	}
}

int main(int argc, char* argv[]) {
	nb__ipc_init("/tmp/ipc_socket", 1);
	printf("IPC initialized\n");

	unsigned int server_id_i = inet_addr(server_id);
	unsigned int client_id_i = inet_addr(client_id);

	nb__my_host_id = server_id_i;
	nb__net_init();


	nb__connection_t * conn = nb__establish(nb__wildcard_host_identifier, 0, 8080, callback);

	while (running) {
		nb__main_loop_step();
		usleep(100 * 1000);
	}
	nb__destablish(conn);
	return 0;
}
