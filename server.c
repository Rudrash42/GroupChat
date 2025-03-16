#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>


int createTCPIpv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }


struct sockaddr_in* createIPv4Address(char *ip, int port) {

    struct sockaddr_in  *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(strlen(ip) ==0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET,ip,&address->sin_addr.s_addr);

    return address;
}

int main(){

  int serverSocketFD = createTCPIpv4Socket();
  struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

  int serverResult = bind(serverSocketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));

  if(serverResult == 0) printf("Socket bound successfully\n");
  else printf("Socket binding failed\n");

  int listenResult = listen(serverSocketFD, 10);
  
  struct sockaddr_in clientAddress;
  int clientAddressSize = sizeof(clientAddress);
  int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &clientAddressSize);

  char buffer[1024];
  recv(clientSocketFD, buffer, 1024,0);

  printf("%s", buffer);

  return 0;

}