#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int createTCPIpv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }

struct sockaddr_in *createIPv4Address(char *ip, int port) {

  struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
  address->sin_family = AF_INET;
  address->sin_port = htons(port);

  if (strlen(ip) == 0)
    address->sin_addr.s_addr = INADDR_ANY;
  else
    inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
  return address;
}

int main() {

  int socketFD = createTCPIpv4Socket();
  struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

  int connectResult =
      connect(socketFD, (struct sockaddr *)address, sizeof(*address));

  if (connectResult == 0)
    printf("Connected Successfully\n");
  else
    printf("Connection Failed\n");

  char *message = NULL;
  size_t lineSize = 0;

  ssize_t charCount = getline() send(socketFD, message, strlen(message), 0);

  char buffer[1024];
  recv(socketFD, buffer, 1024, 0);

  return 0;
}