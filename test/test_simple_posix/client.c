#include "nbp/network.h"
#include "nb_runtime.h"
#include <arpa/inet.h>
#include <stdio.h>

#define SERVER_IP ("10.0.0.1")
#define CLIENT_IP ("10.0.0.2")

#define SERVER_PORT (8080)

int main(int argc, char* argv[]) {	
	nb__ipc_init("/tmp/ipc_socket", 0);
	nb__net_init();
	unsigned long my_ip = inet_addr(CLIENT_IP);	
	memcpy(nb__my_host_id, &my_ip, 6);

	int fd = nbp_socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	srvaddr.sin_port = htons(SERVER_PORT);

	if (nbp_connect(fd, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) != 0) {
		printf("nbp connect failed\n");
		return -1;
	}

	char buffer[1024];
	strcpy(buffer, "Hello from client");
	int len = nbp_send(fd, buffer, strlen(buffer), 0);

	len = nbp_recv(fd, buffer, sizeof(buffer), 0);
	buffer[len] = 0;
	printf("Read: %s\n", buffer);
	
	// Skip closing for now, we don't support it
	return 0;
	
}
