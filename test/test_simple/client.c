#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>

#define CLIENT_MSG ("Hello from client")

char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

int running = 1;
static void callback(int event, nb__connection_t * c) {
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
	nb__net_init();

	unsigned long long server_id_i = 0;
	unsigned long long client_id_i = 0;
	memcpy(&server_id_i, server_id, sizeof(server_id));
	memcpy(&client_id_i, client_id, sizeof(client_id));

	nb__my_host_id = client_id_i;

	nb__connection_t * conn = nb__establish(server_id_i, 8080, 8081, callback);
	
	nb__send(conn, CLIENT_MSG, sizeof(CLIENT_MSG));

	while (running) {
		nb__main_loop_step();
		usleep(100 * 1000);
	}
	nb__destablish(conn);
	return 0;
	
}
