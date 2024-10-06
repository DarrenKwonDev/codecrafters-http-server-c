#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

char* extract_path(const char* request) {
	char* path_start = strchr(request, ' ');
	if (path_start == NULL) {
		return NULL;
	}
	path_start++;  // 공백 다음 문자로 이동

	char* path_end = strchr(path_start, ' ');
    if (path_end == NULL) {
        return NULL;
    }

	int path_length = path_end - path_start;

	// free는 함수 바깥에서 시행 됨.
	char* path = (char*)malloc(path_length + 1);
    if (path == NULL) {
        return NULL;
    }

    strncpy(path, path_start, path_length);
    path[path_length] = '\0';  // 문자열 종료

	return path;
}

char* parse_echo_req(const char* path) {
	/*
		[examples]
		/echo/abcd
		// path는 함수 바깥에서 malloc되어 온 메모리 영역임에 주의.
	*/
	const char* echo_prefix = "/echo/";
    char* echo_start = strstr(path, echo_prefix); // prefix로 시작하는 위치

	// /echo/로 시작하니?
	if (echo_start == path) {
		// 그렇다면 /echo/ 이후의 문자열을 가져와
        char* echo_str = echo_start + strlen(echo_prefix);
        if (*echo_str != '\0') {
            return echo_str;
        }
    }

    return NULL;
}

char* parse_user_agent_req(const char* req) {
	char* is_sa = strstr(req, "/user-agent");
	if (is_sa == NULL) return NULL;

	printf("[parse_user_agent_req] %s \n", req);

	const char* header_start = strstr(req, "\r\n") + 2;  // Skip the request line
    const char* header_end = strstr(header_start, "\r\n\r\n");

	if (header_start == NULL || header_end == NULL) {
        printf("Invalid HTTP request format\n");
        return NULL;
    }

	int headers_length = header_end - header_start;
	char headers[1024] = {0,};
    strncpy(headers, header_start, headers_length);
	printf("copied headers : %s \n", headers);
	
	char* token = strtok(headers, "\r\n");
	while(token != NULL) {
		printf("token : %s \n", token);
		char* separator = strchr(token, ':');

		if (separator != NULL) {
			int key_length = separator - token;
			char tmp[50] = {0,};
			strncpy(tmp, token, key_length);
			
			// User-Agent 와 같다면.
			if (strncmp(tmp, "User-Agent", key_length) == 0) {

				// : 앞 뒤로 공백 존재. 제거.
				separator++;
				while (*separator == ' ') {
					separator++;
				}

				int value_length = strlen(separator);
				char* ret = (char *)malloc(value_length + 1);
                strcpy(ret, separator);
                return ret;
			}
		}

		token = strtok(NULL, "\r\n"); // 후속 호출
	}

	return NULL;
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

	// path 발라내기
	char* path = extract_path(recvBuf);
	if (path == NULL) {
		goto err;
	}

	printf("[DEBUG] whole url : %s | got path : %s ", recvBuf, path);

	// path가 특정 조건에 만족하는지 체크
	char* echo_str = parse_echo_req(path);
	if (echo_str != NULL) {
		printf("[DEBUG] echo str is : %s", echo_str);
	}

	char* ua_value = parse_user_agent_req(recvBuf);

	// path의 특성에 따라 각자 다른 방법으로 응답 보내기
	if (path[0] == '\0' || !strncmp(path, "/", sizeof("/")) ) { // 빈문자거나 / 가 온다면
		char* res = "HTTP/1.1 200 OK\r\n\r\n";
		send(conn_sock_fd, res, strlen(res), 0);
	} else if (echo_str != NULL) {
		char res[1024] = {0};
		sprintf(res, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s", strlen(echo_str), echo_str);
		send(conn_sock_fd, res, strlen(res), 0);
	} else if (ua_value != NULL) {
		char res[1024] = {0};
		printf("[DEBUG] %s \n", ua_value);
		sprintf(res, 
			"HTTP/1.1 200 OK\r\n"   // Skip the request line
			"Content-Type: text/plain\r\n" // header
			"Content-Length: %zu\r\n"
			"\r\n"						   // end header
			"%s", 						// body
			strlen(ua_value), ua_value);
		send(conn_sock_fd, res, strlen(res), 0);
	}
	else {
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		send(conn_sock_fd, res, strlen(res), 0);
	}


	close(conn_sock_fd);
	close(server_fd);
	if (path != NULL) {
		free(path);
	}
	if (ua_value != NULL) {
		free(ua_value);
	}
	return 0;

err:
	if (path != NULL) {
		free(path);
	}
	if (ua_value != NULL) {
		free(ua_value);
	}
	close(conn_sock_fd);
	close(server_fd);
	return -1;
}


