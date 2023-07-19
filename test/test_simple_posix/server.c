#include "nbp/network.h"
#include "nb_runtime.h"
#include <arpa/inet.h>
#include <stdio.h>

#define SERVER_IP ("10.0.0.1")
#define CLIENT_IP ("10.0.0.2")

#define SERVER_PORT (8080)

int main(int argc, char* argv[]) {	
	nb__ipc_init("/tmp/ipc_socket", 1);
	nb__net_init();
	unsigned long my_ip = inet_addr(SERVER_IP);
	memcpy(nb__my_host_id, &my_ip, 6);

	int fd = nbp_socket(AF_INET, SOCK_STREAM, 0);
	//printf("fd created = %d", fd);

	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(SERVER_PORT);

	if (nbp_bind(fd, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) != 0) {
		printf("nbp bind failed\n");
		return -1;
	}
	//printf("bind completed\n");
	if (nbp_listen(fd, 80) != 0) {
		printf("nbp listen failed\n");
		return -1;
	}
	//printf("listen completed\n");
	
	int connfd = nbp_accept(fd, NULL, 0);
	if (connfd < 0) {
		printf("nbp accept failed\n");
		return -1;
	}
	//printf("Accepted new connection = %d\n", connfd);
	
	char buffer[1024];
	int len = nbp_recv(connfd, buffer, sizeof(buffer), 0);
	buffer[len] = 0;
	printf("Read: %s\n", buffer);

	strcpy(buffer, "Hello from server");
	len = nbp_send(connfd, buffer, strlen(buffer), 0);
	
	// Skip closing for now, we don't support it
	return 0;
	
}
