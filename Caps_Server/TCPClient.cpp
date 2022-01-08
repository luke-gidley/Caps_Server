#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "TCPClient.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define PORT_BUFFER_LEN 10
#define DEFAULT_BUFLEN 65536

TCPClient::TCPClient(std::string server, unsigned short int port)
{
	WSADATA wsaData;
	int iResult;
	char portString[PORT_BUFFER_LEN];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	snprintf(portString, PORT_BUFFER_LEN - 1, "%d", port);
	this->portString = portString;
	this->server = server;

	ConnectSocket = INVALID_SOCKET;
}

TCPClient::~TCPClient()
{
	WSACleanup();
}

void TCPClient::OpenConnection()
{
	int iResult;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(server.c_str(), portString.c_str(), &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			exit(1);
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		exit(1);
	}
}

std::string TCPClient::send(std::string request)
{
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN - 1;
	std::string dataReceived;

	//OpenConnection();

	// Send an initial buffer
	iResult = ::send(ConnectSocket, request.c_str(), (int)request.size() + 1, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		exit(1);
	}

	//printf("Bytes Sent: %ld\n", iResult);

	// Wait to receive from server

	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult >= 0)
	{
		recvbuf[iResult] = '\0';
		//printf("Bytes received: %d\n", iResult);
		dataReceived = recvbuf;
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		exit(1);
	}

	//CloseConnection();

	return dataReceived;
}

void TCPClient::CloseConnection()
{
	int iResult;

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		exit(1);
	}

	// cleanup
	closesocket(ConnectSocket);
}
