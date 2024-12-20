#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DIE(s...) {\
	fprintf(stderr, s);\
	exit(1);\
}

const int server_port = 80;

int server_socket;
FILE * index_html;

void close_server_socket(void) {
	close(server_socket);
}

void close_index_html(void) {
	if (index_html) fclose(index_html);
}

int main() {
	atexit(close_index_html);
	atexit(close_server_socket);

	index_html = fopen("srv/index.html", "r");
	if (!index_html) 
		DIE("fopen() failed: %s\n", strerror(errno));

	struct sockaddr_in server_addr;

	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket < 0)
		DIE("socket() failed: %s\n", strerror(errno));

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		DIE("bind() failed: %s\n", strerror(errno));
	}

	if (listen(server_socket, SOMAXCONN) < 0) {
		close(server_socket);
		DIE("listen() failed: %s\n", strerror(errno));
	}
	printf("Listening at localhost:%d\n", server_port);

	int client_socket;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	memset(&client_addr, 0, sizeof(client_addr));

	if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
		DIE("accept() failed: %s\n", strerror(errno));
	}

	char request_buffer[512];
	ssize_t http_request_length;
	do {
		if ((http_request_length = recv(client_socket, request_buffer, 512, 0)) < 0) {
			DIE("recv() failed: %s\n", strerror(errno));
		}

		printf("%.*s", http_request_length, request_buffer);
	} while (http_request_length == 512);
	printf("\n");

	fseek(index_html, 0, SEEK_END);
	long index_html_size = ftell(index_html) + 1;
	fseek(index_html, 0, SEEK_SET);

	char * index_html_data = (char *)malloc(index_html_size);
	fread(index_html_data, sizeof(char), index_html_size, index_html);

	const char * http_response_header = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"\r\n";

	send(client_socket, http_response_header, strlen(http_response_header), 0);
	send(client_socket, index_html_data, index_html_size, 0);

	free(index_html_data);
	return 0;
}
