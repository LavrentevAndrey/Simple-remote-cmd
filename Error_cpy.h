#pragma once
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

void Check_Socket(SOCKET s) {
    if (s == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
}

SOCKET Socket(int af, int type, int protocol) {
    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(af, type, protocol); // �������� ������
    Check_Socket(ConnectSocket);
    return ConnectSocket;
}

int Connect(SOCKET* s, const SOCKADDR* name, int namelen) {
    int iResult = connect(*s, name, namelen); // ������������ �� ������
    if (iResult == SOCKET_ERROR) {
        closesocket(*s);
        *s = INVALID_SOCKET;
    }
    return iResult;
}

void Send(SOCKET s, const char* buf, int len, int flags) {
    int iResult = send(s, buf, len, flags); // ����� ������� �� ������������� ������
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }
    printf("Bytes Sent: %d\n", iResult);
}

int Recv(SOCKET s, char* buf, int len, int flags) {
    int iResult = recv(s, buf, len, flags);
    if (iResult > 0) {
        printf("Bytes received: %d\n", iResult);
    }
    else if (iResult == 0)
        printf("Connection closing...\n");
    else {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
    }
    return iResult;
}

void Getaddrinfo(PCSTR pNode, PCSTR pService, const ADDRINFOA* pHints, PADDRINFOA* ppResult) {
    int iResult = getaddrinfo(pNode, pService, pHints, ppResult); // ��������� ���������� ��� ������� ������ (ip + port) ������������ � result
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(1);
    }
}

void Bind(SOCKET s, ADDRINFO* info) {
    int iResult = bind(s, info->ai_addr, (int)info->ai_addrlen); // ��������� ������ ������������� tcp ������� �� ������
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(info);
        closesocket(s);
        WSACleanup();
        exit(1);
    }
}

void Listen(SOCKET s, int backlog) {
    int iResult = listen(s, backlog); // ����������� � ������ ��� ������������� (1 - ���� ������� �� ���������� � ������)
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }
}

SOCKET Accept(SOCKET s) {
    struct sockaddr_in client_socket; // �������� ���������� �������������� ���������
    int sizeof_addr = sizeof(client_socket); // tmp 

    SOCKET ClientSocket = accept(s, (SOCKADDR*)&client_socket, &sizeof_addr);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }
    else {
        printf("Client connected!\nIP addr of client: ");
        char buffer[INET_ADDRSTRLEN];
        printf("%s\n", inet_ntop(AF_INET, &client_socket.sin_addr, buffer, sizeof(buffer)));
        printf("Client port is: %d\n", client_socket.sin_port);
    }
    return ClientSocket;
}