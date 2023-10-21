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
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
}

SOCKET Socket(int af, int type, int protocol) {
    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(af, type, protocol); // Создание сокета
    Check_Socket(ConnectSocket);
    return ConnectSocket;
}

int Connect(SOCKET* s, const sockaddr* name, int namelen) {
    int iResult = connect(*s, name, namelen); // Подключаемся по сокету
    if (iResult == SOCKET_ERROR) {
        closesocket(*s);
        *s = INVALID_SOCKET;
    }
    return iResult;
}

void Send(SOCKET s, const char* buf, int len, int flags) {
    int iResult = send(s, buf, len, flags); // Обмен данными по подключённому сокету
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }
    printf("Bytes Sent: %ld\n", iResult);
}

int Recv(SOCKET s, char* buf, int len, int flags) {
    int iResult = recv(s, buf, len, flags);
    if (iResult > 0)
        printf("Bytes received: %d\n", iResult);
    else if (iResult == 0)
        printf("Connection closed\n");
    else
        printf("recv failed with error: %d\n", WSAGetLastError());
    return iResult;
}