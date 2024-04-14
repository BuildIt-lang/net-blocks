#include "transport.h"

int main(int argc, char* argv[]) {
        transport_t t;
        mlx5_try_transport(&t, "eth2");


	int counter = 0;

	while(1) {	
		int pending_recv = mlx5_try_recv(&t);
		for (int i = 0; i < pending_recv; i++) {
			
			char* packet_addr = mlx5_get_next_recv_buffer(&t);
			counter++;
			printf("Recv[%d]: %s\n", counter, packet_addr);
		}
	}
	
}
