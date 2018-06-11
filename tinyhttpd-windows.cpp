#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request(SOCKET);
void error_die(const char *);
int get_line(SOCKET, char *, int);
void not_found(SOCKET);
SOCKET startup(const char *);
void unimplemented(SOCKET);

void accept_request(SOCKET client_socket) {

	SOCKET client = client_socket;
	int numchars = 0;

	char buf[1024];
	int iResult;

	numchars = get_line(client, buf, sizeof(buf));
	
	not_found(client);

	iResult = shutdown(client, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		closesocket(client);
		WSACleanup();
		error_die("shutdown failed with error\n");

	}
	closesocket(client);

}

void error_die(const char *sc)
{
	perror(sc);
	exit(1);
}

int get_line(SOCKET sock, char *buf, int size) {
	int i = 0;
	char c = '\0';
	int n;
	while ((i < size - 1) && (c != '\n')) {
		n = recv(sock, &c, 1, 0);
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';
	return i;
}

void not_found(SOCKET client) {
	char buf[1024];
	sprintf_s(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "<BODY><P>The server could not fulfill\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "is unavailable or nonexistent.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

SOCKET startup(char *port) {
	SOCKET httpd = INVALID_SOCKET;

	//initialize winsock
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		error_die("WSAStartup failed\n");
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0) {
		WSACleanup();
		error_die("getaddrinfo failed:\n");
	}

	httpd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (httpd == INVALID_SOCKET) {
		freeaddrinfo(result);
		WSACleanup();
		error_die("Error at socket()\n");
	}

	iResult = bind(httpd, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		freeaddrinfo(result);
		closesocket(httpd);
		WSACleanup();
		error_die("bind failed with error\n");
	}

	if (listen(httpd, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(httpd);
		WSACleanup();
		error_die("Listen failed with error\n");
	}
	return httpd;
}

void unimplemented(SOCKET client)
{
	char buf[1024];

	sprintf_s(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf_s(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

int main(void) {

	SOCKET server_sock = INVALID_SOCKET;
	char port[] = "4000";
	SOCKET client_sock = INVALID_SOCKET;

	server_sock = startup(port);
	printf("httpd running on port %s\n", port);

	while (true) {
		client_sock = accept(server_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET) {
			printf("accept failed:%d\n", WSAGetLastError());
			closesocket(server_sock);
			WSACleanup();
			return 1;
		}
		accept_request(client_sock);
	}
	closesocket(server_sock);
	WSACleanup();
	return 0;
}