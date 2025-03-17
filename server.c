#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

struct acceptedSocket{

  int acceptedSocketFD;
  struct sockaddr_in address;
  int error;
  bool result;

};

struct acceptedSocket acceptedArray[10] ;
int acceptedSocketsCount = 0;

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

struct acceptedSocket* acceptClient(int serverSocketFD){

  struct sockaddr_in clientAddress;
  int clientAddressSize = sizeof(clientAddress);
  int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &clientAddressSize);
  

  struct acceptedSocket* acceptedClientSocket = malloc(sizeof(struct acceptedSocket));

  acceptedClientSocket -> address = clientAddress;
  acceptedClientSocket -> acceptedSocketFD = clientSocketFD;
  acceptedClientSocket -> result = clientSocketFD > 0;
  if(!acceptedClientSocket -> result)acceptedClientSocket -> error = clientSocketFD;

  return acceptedClientSocket;

}

void propogateToThreads(char* buffer, int socketFD){

  for (int i = 0; i < acceptedSocketsCount; i++)
  {
    if(acceptedArray[i].acceptedSocketFD != socketFD){
      send(acceptedArray[i].acceptedSocketFD, buffer, strlen(buffer), 0);
    }
  }
  
}

void receiveandPrintIncomingData(int socketFD){
  char buffer[1024];

  while (true)
  {
    ssize_t amountReceived =   recv(socketFD, buffer, 1024,0);
    if(amountReceived > 0) {
        buffer[amountReceived] = 0;
        printf("%s ", buffer);
        printf("\n");

        propogateToThreads(buffer, socketFD);
    }
    if(amountReceived == 0) break;
  }

    close(socketFD);

  }

void receiveandPrintonThread(struct acceptedSocket* clientSocket){

    pthread_t id;

    // int *clientFD = malloc(sizeof(int));
    // if (clientFD == NULL) {
    //     perror("Memory allocation failed");
    //     return;
    // }
    // *clientFD = clientSocket->acceptedSocketFD;

    pthread_create(&id, NULL, receiveandPrintIncomingData, clientSocket -> acceptedSocketFD);

}

void startConnecting(int serverSocketFD){

  while(true){

    struct acceptedSocket* clientSocket = acceptClient(serverSocketFD);
    acceptedArray[acceptedSocketsCount++] = *clientSocket;

    receiveandPrintonThread(clientSocket);

  }

}


int main(){

  int serverSocketFD = createTCPIpv4Socket();
  struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

  int serverResult = bind(serverSocketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));

  if(serverResult == 0) printf("Socket bound successfully\n");
  else printf("Socket binding failed\n");

  int listenResult = listen(serverSocketFD, 10);

  startConnecting(serverSocketFD); 
  
  close(serverSocketFD);

  return 0;

}