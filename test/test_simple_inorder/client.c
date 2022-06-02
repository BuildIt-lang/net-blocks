#include "nb_runtime.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
extern int nb__ipc_simulate_out_of_order;

char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_READ_READY) {
	}
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	nb__ipc_init("/tmp/ipc_socket", 0);
	printf("IPC initialized\n");
	nb__ipc_simulate_out_of_order = 1;
	nb__net_init();
	memcpy(nb__my_host_id, client_id, 6);

	nb__connection_t * conn = nb__establish(server_id, 8080, 8081, callback);

	char buffer[16];
		
	for (int i = 0; i < 16; i++) 	{
		sprintf(buffer, "%014d\n", i);
		nb__send(conn, buffer, sizeof(buffer));
	}

	int count = 0;
	while (count < 30) {
		nb__main_loop_step();
		usleep(100 * 1000);
		count++;
	}

	nb__destablish(conn);
	nb__ipc_deinit();
	return 0;
	
}
