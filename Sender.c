#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "sockconst.h"

const char * fileName = "sendthis.txt";
const char * CC_reno = "reno";
const char * CC_cubic = "cubic";

int socketSetup(struct sockaddr_in *serverAddress) {
    int socketfd = INVALID_SOCKET;
    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, (const char*) SERVER_IP_ADDRESS, &serverAddress->sin_addr) == -1)
    {
        perror("inet_pton()");
        exit(1);
    }

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        perror("socket()");
        exit(1);
    }

    return socketfd;
}

int authCheck(int socketfd) {
    int buffer = 0, check = AUTH_CHECK;
    printf("Waiting for authentication...\n");

    recv(socketfd, &buffer, sizeof(int), 0);

    if (buffer != check)
    {
        printf("Error with authentication!\n");
        return 0;
    }

    printf("Authentication completed.\n");

    return 1;
}

void sendExitandClose(int socketfd) {
    char* exitcmd = "exit";
    send(socketfd, exitcmd, strlen(exitcmd), 0);

    printf("Closing connection...\n");

    close(socketfd);
}

void readFromFile(char* fileContent) {
    FILE * fpointer = NULL;
    fpointer = fopen(fileName, "r");

    if (fpointer == NULL)
    {
        perror("fopen");
        exit(1);
    }

    fread(fileContent, sizeof(char), FILE_SIZE, fpointer);
    fclose(fpointer);
}

int sendFile(int socketfd, char* buffer, int len) {
    int sentd = send(socketfd, buffer, len, 0);

    if (sentd == 0)
        printf("Server doesn't accept requests.\n");

    else if (sentd < len)
    {
        printf("File was only partly sent (%d/%d bytes).\n", sentd, len);
    }

    else
        printf("Total bytes sent is %d.\n", sentd);

    return sentd;
}

void changeCCAlgorithm(int socketfd, const char * buffer, int len) {
    char buffer2[8];
    socklen_t len2 = sizeof(buffer2);

    if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, buffer, len) != 0)
    {
        perror("setsockopt");
        exit(1);
    }

    if (getsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, buffer2, &len2) != 0)
    {
        perror("getsockopt");
        exit(1);
    }
}

int main() {
    char fileContent[FILE_SIZE];
    int socketfd = INVALID_SOCKET;
    struct sockaddr_in serverAddress;

    printf("Client startup...\n");

    printf("Reading file content\n");
    readFromFile(fileContent);

    printf("Setting up the socket\n");
    socketfd = socketSetup(&serverAddress);

    printf("Connection to %s:%d...\n",SERVER_IP_ADDRESS,SERVER_PORT);

    if (connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        printf("Connection to %s:%d couldn't be made!\n", SERVER_IP_ADDRESS, SERVER_PORT);
        perror("connect");
        exit(1);
    }

    printf("Connected successfully to %s:%d!\n", SERVER_IP_ADDRESS, SERVER_PORT);

    while(1)
    {
        int choice, check = 0;
        socklen_t len = strlen(CC_reno);

        while(!check)
        {
            sendFile(socketfd, fileContent, (FILE_SIZE/2));
            check = authCheck(socketfd);
        }

        check = 0;

        printf("Changing congestion control algorithm to reno...\n");
        
        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, CC_reno, len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }

        while(!check)
        {
            sendFile(socketfd, fileContent, (FILE_SIZE/2));
            check = authCheck(socketfd);
        }

        printf("Send the file again? (For data gathering)\n");
        scanf("%d", &choice);

        if (!choice)
            break;

        printf("Changing congestion control algorithm to cubic...\n");

        len = strlen(CC_cubic);
        
        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, CC_cubic, len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
    
    sleep(1);

    sendExitandClose(socketfd);

    return 0;
}