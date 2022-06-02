#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>

#define SERVER_MSG ("Hello from server")

char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_ACCEPT_READY) {
		// Share the callback function
		nb__connection_t * s = nb__accept(c, callback);
	} else if (event == QUEUE_EVENT_READ_READY) {	
		char buff[65];
		int len = nb__read(c, buff, 64);
		printf("Received = %s", buff);	
	}
}

int main(int argc, char* argv[]) {
	nb__ipc_init("/tmp/ipc_socket", 1);
	printf("IPC initialized\n");
	nb__net_init();
	memcpy(nb__my_host_id, server_id, 6);

	nb__connection_t * conn = nb__establish(nb__wildcard_host_identifier, 0, 8080, callback);
	
	int count = 0;
	while (count < 50000) {
		nb__main_loop_step();
		usleep(1 * 100);
		count++;
	}
	nb__destablish(conn);
	nb__ipc_deinit();
	return 0;
}
