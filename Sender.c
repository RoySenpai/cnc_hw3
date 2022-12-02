#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "sockconst.h"

int socketfd = INVALID_SOCKET;
struct sockaddr_in serverAddress;

void socketSetup() {
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, (const char*) SERVER_IP_ADDRESS, &serverAddress.sin_addr) == -1)
    {
        perror("inet_pton()");
        exit(1);
    }

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        perror("socket()");
        exit(1);
    }
}

void authCheck(int socketfd) {
    int buffer, check = AUTH_CHECK;

    printf("Waiting for authentication...\n");

    recv(socketfd, &buffer, sizeof(int), 0);

    if (buffer == check)
        printf("Authentication completed.\n");

    else
        printf("Error with authentication!\n");
}

int main() {
    printf("Client startup...\n");

    socketSetup();

    if (connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        printf("Connection to %s:%d couldn't be made!\n",SERVER_IP_ADDRESS,SERVER_PORT);
        perror("connect()");
        exit(1);
    }

    printf("Connected successfully to %s:%d!\n",SERVER_IP_ADDRESS,SERVER_PORT);

    char msg[] = "Testing";
    int msglen = strlen(msg) + 1;

    int sentd = send(socketfd, &msg, msglen, 0);

    if (sentd == 0)
        printf("Server doesn't accept requests.\n");

    else if (sentd < msglen)
        printf("Message was only partly sent (%d/%d bytes)\n", sentd, msglen);

    else
    {
        printf("Message successfuly sent.\n");
        authCheck(socketfd);
    }

    sleep(10);

    printf("Closing connection...\n");

    close(socketfd);

    return 0;
}