# client_server_cmd_local
## Building
- All libs are linked automaticly with msvc. If you want to use gcc you need to manualy link lib
  - gcc -Wextra -Wall client.c -lWs2_32 -o client
  - gcc -Wextra -Wall -lWs2_32 server.c -o server
## Starting
- Litteraly (in correct order):
  - ./server
  - ./client ip_addr
- ip addr - 127.0.0.1 or other ip interface on local machine 
