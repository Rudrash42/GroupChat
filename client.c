#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int createTCPIpv4Socket(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in *createIPv4Address(const char *ip, int port) {
    struct sockaddr_in *address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    if (ip && *ip)
        inet_pton(AF_INET, ip, &address->sin_addr);
    else
        address->sin_addr.s_addr = htonl(INADDR_ANY);
    return address;
}

static void *listenOnThreads(void *arg) {
    int socketFD = *(int *)arg;
    free(arg);

    char buffer[1025]; // +1 for terminator
    for (;;) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);
        if (amountReceived <= 0) break;
        buffer[amountReceived] = 0;
        printf("%s\n", buffer);
    }
    close(socketFD);
    return NULL;
}

void startListening(int socketFD) {
    pthread_t id;
    int *fdp = (int *)malloc(sizeof(int));
    *fdp = socketFD;
    pthread_create(&id, NULL, listenOnThreads, fdp);
    pthread_detach(id);
}

int main(void) {
    const char *server_ip = "127.0.0.1";
    int server_port = 5000; // change if needed

    int socketFD = createTCPIpv4Socket();
    if (socketFD < 0) { perror("socket"); return 1; }

    struct sockaddr_in *address = createIPv4Address(server_ip, server_port);
    int connectResult = connect(socketFD, (struct sockaddr *)address, sizeof(*address));
    free(address);

    if (connectResult == 0) printf("Connected Successfully\n");
    else { perror("connect"); return 1; }

    char *name = NULL; size_t nameSize = 0;
    printf("please enter your name?\n");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    if (nameCount > 0 && name[nameCount - 1] == '\n') name[nameCount - 1] = 0;

    printf("Enter your message\n");
    startListening(socketFD);

    char *message = NULL; size_t messageSize = 0;
    char buffer[2048];

    for (;;) {
        ssize_t charCount = getline(&message, &messageSize, stdin);
        if (charCount <= 0) break;
        if (message[charCount - 1] == '\n') message[charCount - 1] = 0;

        if (strcmp(message, "exit") == 0) break;

        // Send as "name: message\n" (newline-delimited)
        int n = snprintf(buffer, sizeof(buffer), "%s: %s\n", name ? name : "anon", message);
        if (n > 0) send(socketFD, buffer, (size_t)n, 0);
    }

    // Optional: send a terminating 0 int for server's binary path (if you use that).
    // int zero = 0; send(socketFD, &zero, sizeof(zero), 0);

    free(name);
    free(message);
    // listener thread closes the socket when server ends; otherwise close here if you prefer:
    // close(socketFD);
    return 0;
}
