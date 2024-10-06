#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	int server_fd;
	
	// tcp 소켓을 만듭니다.
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { 
		.sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	
	struct sockaddr_in client_addr;
	int client_addr_len;
	client_addr_len = sizeof(client_addr);
	int conn_sock_fd;
	
	printf("Waiting for a client to connect...\n");
	conn_sock_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *)&client_addr_len);
	printf("Client connected\n");

	/*
	[http response]
	// Status line
	HTTP/1.1  // HTTP version
	200       // Status code
	OK        // Optional reason phrase
	\r\n      // CRLF that marks the end of the status line

	// Headers (empty)
	\r\n      // CRLF that marks the end of the headers

	// Response body (empty)
	*/
	char* res = "HTTP/1.1 200 OK\r\n\r\n";
	send(conn_sock_fd, res, strlen(res), 0);
	
	close(server_fd);

	return 0;
}
