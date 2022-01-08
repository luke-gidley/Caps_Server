//#include "stdafx.h"

#include <iostream>

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "TCPServer.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define PORT_BUFFER_LEN 10
#define DEFAULT_BUFLEN 65536

TCPServer::TCPServer(unsigned short int port)
{
	WSADATA wsaData;
	char portString[PORT_BUFFER_LEN];
	int iResult;

	this->port = port;

	snprintf(portString, PORT_BUFFER_LEN - 1, "%d", port);
	this->portString = portString;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	this->OpenListenSocket();
}

void TCPServer::OpenListenSocket()
{
	int iResult;

	ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, portString.c_str(), &hints, &result);
	//iResult = getaddrinfo("127.0.0.1", portString.c_str(), &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
}

ReceivedSocketData TCPServer::accept()
{
	SOCKET ClientSocket = INVALID_SOCKET;
	ReceivedSocketData ret;

	ret.ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	ClientSocket = ::accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	ret.ClientSocket = ClientSocket;

	return ret;
}

void TCPServer::receiveData(ReceivedSocketData &ret, bool blocking)
{
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN - 1;
	struct pollfd pollDescriptor;

	pollDescriptor.fd = ret.ClientSocket;
	pollDescriptor.events = POLLIN;
	pollDescriptor.revents = 0;

	int retval;

	ret.request = "";
	ret.reply = "";

	if (!blocking)
		retval = WSAPoll(&pollDescriptor, 1, 100); //timeout in ms

	if (blocking || (!blocking && retval > 0))
	{
		iResult = recv(ret.ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			recvbuf[iResult] = '\0';
			ret.request = std::string(recvbuf);
		}
		//	else if (iResult == 0)
		//	{
		//		recvbuf[iResult] = '\0';
		//		printf("Received empty string. Terminating server.\n");
		//		ret.request = "";
		//	}
		else if (iResult < 0)
		{
			int SocketError = WSAGetLastError();

			if (SocketError == WSAESHUTDOWN || SocketError == WSAECONNRESET || SocketError == WSAECONNABORTED || SocketError == WSAENETRESET) //connection has been closed, terminated, aborted or reset
			{
				closesocket(ret.ClientSocket);
				ret.ClientSocket = INVALID_SOCKET;
			}
			else if (SocketError != WSAEWOULDBLOCK)
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ret.ClientSocket);
				ret.ClientSocket = INVALID_SOCKET;
				WSACleanup();
				exit(1);
			}
		}
	}
}

int TCPServer::sendReply(ReceivedSocketData reply)
{
	int iSendResult;

	// Echo the buffer back to the sender
	iSendResult = send(reply.ClientSocket, reply.reply.c_str(), (int)reply.reply.size()+1, 0);

	if (iSendResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(reply.ClientSocket);
		WSACleanup();
		return 1;
	}

	return iSendResult;
}

int TCPServer::closeClientSocket(ReceivedSocketData &reply)
{
	int iResult;

	// shutdown the connection since we're done
	iResult = shutdown(reply.ClientSocket, SD_SEND);

	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(reply.ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(reply.ClientSocket);

	return iResult;
}

void TCPServer::CloseListenSocket()
{
	// No longer need server socket
	closesocket(ListenSocket);
}


TCPServer::~TCPServer()
{
	CloseListenSocket();
	WSACleanup();
}


