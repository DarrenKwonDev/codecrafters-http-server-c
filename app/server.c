#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

char* extract_url(const char* request) {
	char* path_start = strchr(request, ' ');
	if (path_start == NULL) {
		return NULL;
	}
	path_start++;  // 공백 다음 문자로 이동

	char* path_end = strchr(path_start, ' ');
    if (path_end == NULL) {
        return NULL;  // 잘못된 형식의 요청
    }

	int path_length = path_end - path_start;

	// free는 함수 바깥에서.
	char* path = (char*)malloc(path_length + 1);
    if (path == NULL) {
        return NULL;  // 메모리 할당 실패
    }

    strncpy(path, path_start, path_length);
    path[path_length] = '\0';  // 문자열 종료

	return path;
}

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

	------------------------------------------------------

	[http request]
	// Request line
	GET                          // HTTP method
	/index.html                  // Request target
	HTTP/1.1                     // HTTP version
	\r\n                         // CRLF that marks the end of the request line

	// Headers (각각의 헤더마다 \r\n을 넣어줘야함)
	Host: localhost:4221\r\n     // Header that specifies the server's host and port
	User-Agent: curl/7.64.1\r\n  // Header that describes the client's user agent
	Accept: *//*\r\n              // Header that specifies which media types the client can accept
	\r\n                         // CRLF that marks the end of the headers

	// Request body (empty)
	*/

	char recvBuf[1024] = {0,};
	recv(conn_sock_fd, recvBuf, sizeof(recvBuf), 0);

	char* path = extract_url(recvBuf);
	printf("[DEBUG] whole url : %s | got path : %s ", recvBuf, path);

	if (!strcmp(path, "") || !strcmp(path, "/") ) { // 같다면
		char* res = "HTTP/1.1 200 OK\r\n\r\n";
		send(conn_sock_fd, res, strlen(res), 0);
		
		close(server_fd);
		free(path);
	} else {
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		send(conn_sock_fd, res, strlen(res), 0);
		
		close(server_fd);
		free(path);
	}

	




	return 0;
}

