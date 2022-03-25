#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>

#define SERVER_MSG ("Hello from server")

char client_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x36, 0x33};
char server_id[] = {0x98, 0x03, 0x9b, 0x9b, 0x2e, 0xeb};

int running = 1;
static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_READ_READY) {
		char buff[1025];
		int len = nb__read(c, buff, 1024);
		nb__send(c, buff, len);
	}
}

int main(int argc, char* argv[]) {
	nb__mlx5_init();
	nb__net_init();
	memcpy(nb__my_host_id, server_id, 6);

	nb__connection_t * conn = nb__establish(client_id, 8081, 8080, callback);

	while (running) {
		nb__main_loop_step();
	}
	nb__destablish(conn);
	return 0;
}
