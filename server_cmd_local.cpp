#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Error_cpy.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "29015"
#define INET_EXIT_STR "exit"
#define INET_EXIT_STR_OK "exit_ok"

int __cdecl main(void)
{
    WSADATA wsaData; // Метаданные сетевых возможностей
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo hints; // Нода в связном списке, нужная для создания структуры с выделенным ip для создания сокета
    struct addrinfo* result = NULL; // Результирующая структура с описанием сокета

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Загрузка библиотеки с версией 2.2, и просто служебная структура
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints)); // memset нулями 
    hints.ai_family = AF_INET; // семейство протоколов
    hints.ai_socktype = SOCK_STREAM; // Самый простой вид взаимодействия с сокетом (ip + port)
    hints.ai_protocol = IPPROTO_TCP; // Спецификация протокола tcp для сетевого взаимодействия
    hints.ai_flags = AI_PASSIVE; // 

    // Resolve the server address and port
    Getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); // Получения информации для содания сокета

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = Socket(result->ai_family, result->ai_socktype, result->ai_protocol); // Создание сокета

    // Setup the TCP listening socket
    Bind(ListenSocket, result); // Установка режима прослушивания tcp трафика на сокете

    freeaddrinfo(result); // Очищаем вспомогательную инфу

    Listen(ListenSocket, 1); // Подключение к сокету для прослушивания (1 - макс очередь на поключение к сокету)

    // Accept a client socket
    ClientSocket = Accept(ListenSocket); // Функция принимает входящее соединение и записывает информацию о нём в client_socket

    // No longer need server socket
    closesocket(ListenSocket); // 

    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];
    memset(recvbuf, 0, DEFAULT_BUFLEN);
    memset(sendbuf, 0, DEFAULT_BUFLEN); // Очищаем буферы для корректного принятия информации
    // Receive until the peer shuts down the connection
    do {
        // Получаем данные по сокету и записываем в буфер
        iResult = Recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (strcmp(recvbuf, INET_EXIT_STR) == 0)
            strcpy_s(sendbuf, INET_EXIT_STR_OK);
        else
            strcpy_s(sendbuf, recvbuf);
        Send(ClientSocket, sendbuf, DEFAULT_BUFLEN, 0);
    } while (iResult > 0 && (strcmp(recvbuf, INET_EXIT_STR) != 0));

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}