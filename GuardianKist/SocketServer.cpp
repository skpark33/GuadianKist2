#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include <SocketServer.h>
#include <TraceLog.h>  
#include "GuardianDlg.h"
#include "winsock.h"


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512

CSocketServer::CSocketServer(CGuardianDlg* dlg, CString port)
: m_dlg(dlg)
, m_port(port)
{
	ClientSocket = INVALID_SOCKET;
}

CSocketServer::~CSocketServer()
{
	Stop();
}

int CSocketServer::Start()
{
	SOCKET ListenSocket = INVALID_SOCKET;
	WSADATA wsaData;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		TraceLog(("WSAStartup failed with error: %d\n", iResult));
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, m_port, &hints, &result);
	if (iResult != 0) {
		TraceLog(("getaddrinfo failed with error: %d\n", iResult));
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for the server to listen for client connections.
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		TraceLog(("socket failed with error: %ld\n", WSAGetLastError()));
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind((SOCKET)ListenSocket, (const struct sockaddr *)result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		TraceLog(("bind failed with error: %d\n", WSAGetLastError()));
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		TraceLog(("listen failed with error: %d\n", WSAGetLastError()));
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		TraceLog(("accept failed with error: %d\n", WSAGetLastError()));
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// No longer need server socket
	closesocket(ListenSocket);
	return 0;
}

int CSocketServer::Receive()
{
	if (ClientSocket == INVALID_SOCKET) return 1;

	int iResult;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			TraceLog(("Bytes received: %d\n", iResult));

			CString retval = m_dlg->SocketReceived(CString(recvbuf));
			Send(retval);
		
		}
		else if (iResult == 0) {
			TraceLog(("Connection closing...\n"));
		} 
		else  {
			TraceLog(("recv failed with error: %d\n", WSAGetLastError()));
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);
	return 0;
}

int CSocketServer::Send(CString& sendStr)
{
	if (ClientSocket == INVALID_SOCKET) return 1;
	TraceLog(("skpark Send(%s)", sendStr));
	int iSendResult = send(ClientSocket, sendStr, sendStr.GetLength(), 0);
	TraceLog(("skpark Send(%s)", sendStr));
	if (iSendResult == SOCKET_ERROR) {
		TraceLog(("send failed with error: %d\n", WSAGetLastError()));
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	TraceLog(("Bytes sent: %d\n", iSendResult));
	return 0;
}

int CSocketServer::Stop()
{
	if (ClientSocket == INVALID_SOCKET) return 1;

	// shutdown the connection since we're done
	int iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		TraceLog(("shutdown failed with error: %d\n", WSAGetLastError()));
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}


