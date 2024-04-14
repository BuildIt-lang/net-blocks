#include "transport.h"


char msg_buffer[1024];

void send_message(transport_t *t) {
	char* buffer = mlx5_get_next_send_buffer(t);
	memcpy(buffer, msg_buffer, 257);	
	mlx5_try_send(t, buffer, 257);		
}

int main(int argc, char* argv[]) {
        transport_t t;
        mlx5_try_transport(&t, "eth2");

	memset(msg_buffer, 'x', 256);	
	msg_buffer[256] = 0;
	
	for (int i = 0; i < 1024; i++) {
		send_message(&t);
	}
	
}
