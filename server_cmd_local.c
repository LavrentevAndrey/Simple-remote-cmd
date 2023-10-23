#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include "Error_cpy.h"
// #include "pipe_cmd_server.h"

// Need to link with Ws2_32.lib
#if defined(_MSC_VER) || defined(__BORLANDC__)
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
#endif

#define DEFAULT_BUFSIZE 4096
#define DEFAULT_PORT "29015"
#define INET_EXIT_STR "exit"
#define INET_EXIT_STR_OK "exit_ok"
#define DEFAULT_CMD_PATH "\\\\?\\C:\\Windows\\System32\\cmd.exe"

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess();

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data.
void WriteToPipe(CHAR chBuf[DEFAULT_BUFSIZE]);

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data.
void ReadFromPipe(CHAR chBuf[DEFAULT_BUFSIZE]);

// Format a readable error message, display a message box, 
// and exit from the application.
void ErrorExit(PTSTR lpszFunction);

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

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

    // PIPE AND CMD SUBPROCESS GENERATION PART
    SECURITY_ATTRIBUTES saAttr; 
 
    printf("\n->Start of parent execution.\n");

    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 
    if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
        ErrorExit(TEXT("StdoutRd CreatePipe")); 

    // Ensure the read handle to the pipe for STDOUT is not inherited
    if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
        ErrorExit(TEXT("Stdout SetHandleInformation")); 

    // Create a pipe for the child process's STDIN. 
    if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
        ErrorExit(TEXT("Stdin CreatePipe")); 

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
        ErrorExit(TEXT("Stdin SetHandleInformation")); 
 
    // Create the child process. 
    CreateChildProcess();

    // No longer need server socket
    closesocket(ListenSocket); // 

    char recvbuf[DEFAULT_BUFSIZE];
    char sendbuf[DEFAULT_BUFSIZE];
    memset(recvbuf, 0, DEFAULT_BUFSIZE);
    memset(sendbuf, 0, DEFAULT_BUFSIZE); // Очищаем буферы для корректного принятия информации
    // Receive until the peer shuts down the connection
    do {
        // Получаем данные по сокету и записываем в буфер
        iResult = Recv(ClientSocket, recvbuf, DEFAULT_BUFSIZE, 0);
        printf("\nRESIVED buf: %s\n", recvbuf);
        if (strcmp(recvbuf, INET_EXIT_STR) == 0)
            strcpy_s(sendbuf, sizeof(INET_EXIT_STR_OK), INET_EXIT_STR_OK);
        else {
            strcpy_s(sendbuf, DEFAULT_BUFSIZE, recvbuf); // Копирую принимающий буфер в отправляющий
            WriteToPipe(sendbuf); //sending command to console
            ReadFromPipe(sendbuf); //reading console output
        }
        Send(ClientSocket, sendbuf, DEFAULT_BUFSIZE, 0);
    } while (iResult > 0 && (strcmp(recvbuf, INET_EXIT_STR) != 0));

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
        ErrorExit(TEXT("StdInWr CloseHandle\n"));
    if ( ! CloseHandle(g_hChildStd_IN_Rd) ) 
        ErrorExit(TEXT("StdInRD CloseHandle\n"));
    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
   TCHAR szCmdline[]=TEXT(DEFAULT_CMD_PATH);
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
 
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    
   bSuccess = CreateProcess(NULL, 
      szCmdline,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      ErrorExit(TEXT("CreateProcess"));
   else 
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example. 

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      
      // Close handles to the stdin and stdout pipes no longer needed by the child process.
      // If they are not explicitly closed, there is no way to recognize that the child process has ended.
      
      CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_IN_Rd);
   }
}
 
void WriteToPipe(CHAR chBuf[DEFAULT_BUFSIZE])
// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{ 
   DWORD dwWritten = 0;
   BOOL bSuccess = FALSE;
   printf("\nchBuf: %s\nHandle: %p\n", chBuf, g_hChildStd_IN_Wr);
   bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, DEFAULT_BUFSIZE, &dwWritten, NULL);
   printf("bSuccess: %d\tdwRead: %lu\n", bSuccess, dwWritten);
   if ( ! bSuccess || dwWritten == 0 )
      printf("\nSomthing went wrong in sendig data to pipe\n");
   else
      printf( "\n->Contents of {%s} written to child STDIN pipe.\n", chBuf);
    // Close the pipe handle so the child process stops reading. 
    if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
        ErrorExit(TEXT("StdInWr CloseHandle\n"));
} 
 
void ReadFromPipe(CHAR chBuf[DEFAULT_BUFSIZE]) 
// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   BOOL bSuccess = FALSE;
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

   for (;;) 
   { 
      bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, DEFAULT_BUFSIZE, &dwRead, NULL);
      if( ! bSuccess || dwRead == 0 ) break; 

      bSuccess = WriteFile(hParentStdOut, chBuf, 
                           dwRead, &dwWritten, NULL);
      if (! bSuccess ) break; 
   } 
} 
 
void ErrorExit(PTSTR lpszFunction) 

// Format a readable error message, display a message box, 
// and exit from the application.
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}
