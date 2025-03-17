#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

void listenOnThreads(int socketFD) {

  char buffer[1024];

  while (true) {
    ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

    if (amountReceived > 0) {
      buffer[amountReceived] = 0;
      printf("%s ", buffer);
      printf("\n");
    }

    if (amountReceived == 0)
      break;
  }

  close(socketFD);
}

void startListening(int socketFD) {

  pthread_t id;
  pthread_create(&id, NULL, listenOnThreads, socketFD);
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

  char *name = NULL;
  size_t nameSize = 0;
  printf("please enter your name?\n");
  ssize_t nameCount = getline(&name, &nameSize, stdin);
  name[nameCount - 1] = 0;

  char *message = NULL;
  size_t messageSize = 0;
  printf("Enter your message\n");

  startListening(socketFD);

  char buffer[1024];

  while (true) {
    ssize_t charCount = getline(&message, &messageSize, stdin);
    message[charCount - 1] = 0;

    sprintf(buffer, "%s: %s", name, message);

    if (charCount > 0 && strcmp(message, "exit") == 0)
      break;
    else if (charCount > 0) {
      ssize_t amountSent = send(socketFD, buffer, strlen(buffer), 0);
    }
  }

  close(socketFD);

  return 0;
}