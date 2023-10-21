#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Error_handler.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "29015"
#define INET_EXIT_STR "exit_ok"

int __cdecl main(int argc, char** argv)
{
    WSADATA wsaData; // Метаданные сетевых возможностей
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints; // Ноды в связном списке, нужные для создания ip сокета
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Загрузка библиотеки с версией 2.2, и просто служебная структура
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints)); // memset нулями 
    hints.ai_family = AF_UNSPEC; // Неопределённое семейство протоколов
    hints.ai_socktype = SOCK_STREAM; // Самый простой вид взаимодействия с сокетом (ip + port)
    hints.ai_protocol = IPPROTO_TCP; // Спецификация протокола tcp для сетевого взаимодействия

    // Resolve the server address and port
    Getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result); // Получения информации для содания сокета (ip + port) записываются в result

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) { // Пробуем в цикле подключится ко всем сокетам в листе

        // Create a SOCKET for connecting to server
        ConnectSocket = Socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // Создание сокета

        // Connect to server.
        Connect(&ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen); //Подключение по сокету (Syn/ack)
    }

    freeaddrinfo(result); // Очищаем вспомогательную инфу

    Check_Socket(ConnectSocket); // Проверяем получилось ли у нас в иьтоге подключиться
    printf("Print: \"exit\"  to end session\n");
    // Receive until the peer closes the connection
    do { // Основной цикл обмена сообщениями
        // Send buffer
        for (int i = 0; ; ++i) { // Считывание символов с клавиатуры
            unsigned char c = (unsigned char)_getch();
            sendbuf[i] = c;
            if (c == '\r') { // Перенос строки
                printf("\n");
                sendbuf[i] = '\0';
                break;
            }
            else if(c == '\b' && i > 0) { // Delete 
                sendbuf[i] = '\0';
                sendbuf[i - 1] = '\0';
                i--;
                continue;
            }
            if (i == DEFAULT_BUFLEN - 2) {
                sendbuf[i + 1] = '\0';
                printf("\nNOTE: Reached max length of bufer\n");
                break;
            }
            printf("%c", c);
        }
        Send(ConnectSocket, sendbuf, DEFAULT_BUFLEN, 0); // Обмен данными по подключённому сокету
        // Получаем данные по сокету и записываем в буфер
        iResult = Recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        printf("%s\n", recvbuf);
    } while (iResult > 0 && (strcmp(recvbuf, INET_EXIT_STR) != 0)); // работаем пока сервер не подтвердит выход

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND); // Отключения соединения сокетов
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}