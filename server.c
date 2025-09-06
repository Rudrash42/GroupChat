#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

int server_number, maxConnect;

static ssize_t read_line(int fd, char *buf, size_t max) {
    size_t i = 0;
    while (i + 1 < max) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) return 0;                 // peer closed
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

static int read_full(int fd, void *out, size_t need) {
    char *p = (char *)out;
    size_t got = 0;
    while (got < need) {
        ssize_t n = recv(fd, p + got, need - got, 0);
        if (n == 0) return 0;                 // closed
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        got += (size_t)n;
    }
    return 1;
}

static void *socketThread(void *arg) {
    int clientSock = *((int *)arg);
    free(arg);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    getpeername(clientSock, (struct sockaddr *)&client_addr, &client_len);

    char *cli_ip = inet_ntoa(client_addr.sin_addr);
    int cli_port = ntohs(client_addr.sin_port);

    printf("\nClient connected from %s:%d\n", cli_ip, cli_port);

    for (;;) {
        // Peek first byte: if text 'P', handle PING line; else use int path
        char first;
        ssize_t pn = recv(clientSock, &first, 1, MSG_PEEK);
        if (pn <= 0) break;

        if (first == 'P') {
            char line[256];
            ssize_t ln = read_line(clientSock, line, sizeof(line));
            if (ln <= 0) break;

            int cid; long long ts;
            if (sscanf(line, "PING %d %lld", &cid, &ts) == 2) {
                char out[64];
                int len = snprintf(out, sizeof(out), "PONG %d %lld\n", cid, ts);
                send(clientSock, out, len, 0);
                continue;
            }
            // Unknown text command: ignore
            continue;
        }

        // Binary int path (your original behavior)
        int clientNum;
        int rf = read_full(clientSock, &clientNum, sizeof(clientNum));
        if (rf <= 0) break;

        printf("Server received from Client (%s:%d) = %d\n", cli_ip, cli_port, clientNum);

        if (clientNum == 0) {
            sleep(1);
            printf("Client %s:%d disconnected. ", cli_ip, cli_port);
            break;
        }

        int prd = server_number * clientNum;
        double div = (double)server_number / clientNum;

        send(clientSock, &prd, sizeof(prd), 0);
        send(clientSock, &div, sizeof(div), 0);
    }

    printf("Closing connection from server-side.\n");
    close(clientSock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 7 || strcmp(argv[1], "-i") != 0 || strcmp(argv[3], "-p") != 0 || strcmp(argv[5], "-c") != 0) {
        printf("Incorrect usage!\nCorrect: ./server -i <IP> -p <port> -c <integer>\n");
        return 1;
    }

    char *ip_add = argv[2];
    int portNum = atoi(argv[4]);
    server_number = atoi(argv[6]);

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNum);
    if (inet_pton(AF_INET, ip_add, &serverAddress.sin_addr) <= 0) {
        perror("Invalid IP address");
        return 1;
    }

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(serverSock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("bind");
        return 1;
    }

    printf("\nServer bound to:\n\tIP: %s\n\tPort: %d\n", ip_add, portNum);

    maxConnect = 2;
    if (listen(serverSock, maxConnect) != 0) {
        perror("listen");
        return 1;
    }
    printf("\nListening...\n\n");

    for (;;) {
        struct sockaddr_storage clientStorage;
        socklen_t addressSize = sizeof(clientStorage);
        int newSocket = accept(serverSock, (struct sockaddr *)&clientStorage, &addressSize);
        if (newSocket < 0) { perror("accept"); continue; }

        int *fdptr = (int *)malloc(sizeof(int));
        if (!fdptr) { close(newSocket); continue; }
        *fdptr = newSocket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, socketThread, fdptr) != 0) {
            perror("pthread_create");
            close(newSocket);
            free(fdptr);
            continue;
        }
        pthread_detach(tid); // no joins needed
    }

    return 0;
}
