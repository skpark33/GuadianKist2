#include "stdafx.h"
#include "SocketClient.h"
#include <TraceLog.h>  

#define WIN32_LEAN_AND_MEAN

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512

CSocketClient::CSocketClient(CString serverIp, CString port)
: m_port(port)
, m_serverIp(serverIp)
{
	ConnectSocket = INVALID_SOCKET;
}

CSocketClient::~CSocketClient()
{
	Stop();
}

int CSocketClient::Connect()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	//char recvbuf[DEFAULT_BUFLEN];
	// int iResult;
	//int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		TraceLog(("WSAStartup failed with error: %d\n", iResult));
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(m_serverIp, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		TraceLog(("getaddrinfo failed with error: %d\n", iResult));
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			TraceLog(("socket failed with error: %ld\n", WSAGetLastError()));
			WSACleanup();
			return 1;
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
		TraceLog(("Unable to connect to server!\n"));
		WSACleanup();
		return 1;
	}
	return 0;
}
int CSocketClient::Stop()
{
	int iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		TraceLog(("shutdown failed with error: %d\n", WSAGetLastError()));
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}
int CSocketClient::Send(CString sendbuf)
{
	int iResult = send(ConnectSocket, sendbuf, sendbuf.GetLength(), 0);
	if (iResult == SOCKET_ERROR) {
		TraceLog(("send failed with error: %d\n", WSAGetLastError()));
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
}
int CSocketClient::Receive()
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult = 0;
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			TraceLog(("Bytes received: %d\n", iResult));
		}
		else if (iResult == 0) {
			TraceLog(("Connection closed\n"));
		}
		else {
			TraceLog(("recv failed with error: %d\n", WSAGetLastError()));
		}
	} while (iResult > 0);
	return iResult;
}
