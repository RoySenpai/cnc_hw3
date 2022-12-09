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
#include <time.h>
#include "sockconst.h"

char * timestamp();
#define printf_time(f_, ...) printf("%s ", timestamp()), printf((f_), ##__VA_ARGS__);

char * timestamp() {
    char buffer[16];
    time_t now = time(NULL);
    struct tm* timest = localtime(&now);

    strftime(buffer, sizeof(buffer), "(%H:%M:%S)", timest);

    char * time = buffer;

    return time;
}

int socketSetup(struct sockaddr_in *serverAddress) {
    int socketfd = INVALID_SOCKET, canReused = 1;

    memset(serverAddress, 0, sizeof(*serverAddress));
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_addr.s_addr = INADDR_ANY;
    serverAddress->sin_port = htons(SERVER_PORT);

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        perror("socket");
        exit(1);
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &canReused, sizeof(canReused)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    if (bind(socketfd, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(socketfd, MAX_QUEUE) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf_time("Socket successfully created.\n");

    return socketfd;
}

int getDataFromClient(int clientSocket, char *buffer, int len) {
    int recvb = recv(clientSocket, buffer, len, 0);

    if (recvb == -1)
    {
        perror("recv()");
        exit(1);
    }

    else if (!recvb)
    {
        printf_time("Connection with client closed.\n");
        return 0;
    }

    return recvb;
}

int main() {
    int socketfd = INVALID_SOCKET;
    int running = 1;
    struct sockaddr_in serverAddress;

    printf_time("Server starting up...\n");

    socketfd = socketSetup(&serverAddress);

    printf_time("Listening on %s:%d...\n", SERVER_IP_ADDRESS, SERVER_PORT);

    while(running)
    {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen;
        int clientSocket;
        char clientAddr[INET_ADDRSTRLEN];

        memset(&clientAddress, 0, sizeof(clientAddress));

        clientAddressLen = sizeof(clientAddress);

        clientSocket = accept(socketfd, (struct sockaddr *) &clientAddress, &clientAddressLen);

        if (clientSocket == -1)
        {
            perror("accept");
            exit(1);
        }
        
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET_ADDRSTRLEN);

        printf_time("Connection made with {%s:%d}\n", clientAddr, clientAddress.sin_port);

        while (1)
        {
            int totalBytesReceived, buff = AUTH_CHECK;
            char buffer[FILE_SIZE/2];

            memset(buffer, 0, sizeof(buffer));

            printf_time("Waiting for client data...\n");

            totalBytesReceived = getDataFromClient(clientSocket, buffer, (FILE_SIZE/2));

            if (buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't')
            {
                printf_time("Exit command received, exiting...\n");
                running = 0;
                break;
            }

            printf_time("Received total %d/%d bytes\n", totalBytesReceived, (FILE_SIZE/2));

            send(clientSocket, &buff, sizeof(int), 0);

            printf_time("Authentication sent back.\n");
        }

        sleep(1);
        close(clientSocket);
        printf_time("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);
    }

    close(socketfd);
    printf_time("Server shutdown...\n");
    
    return 0;
}