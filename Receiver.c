/*
 *  Communication and Computing Course Assigment 3:
 *  TCP – Congestion Control Algorithms Network programming in C
 *  Copyright (C) 2022  Roy Simanovich and Yuval Yurzdichinsky

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
        perror("recv");
        exit(1);
    }

    else if (!recvb)
    {
        printf_time("Connection with client closed.\n");
        return 0;
    }

    return recvb;
}

void calculateTimes(clock_t * firstPart, clock_t * secondPart, int times_runned) {
    clock_t sumFirstPart = 0, sumSecondPart = 0, avgFirstPart, avgSecondPart;

    printf("--------------------------------------------\n");
    printf("(*) Times summary:\n");

    for (int i = 0; i < times_runned; ++i)
    {
        sumFirstPart += firstPart[i];
        sumSecondPart += secondPart[i];

        printf("(*) Run %d: First part: %ld μs; Second part: %ld μs\n", (i+1), (firstPart[i]), (secondPart[i]));
    }

    avgFirstPart = (sumFirstPart / times_runned);
    avgSecondPart = (sumSecondPart / times_runned);

    printf("(*) Time avarages:\n");
    printf("(*) First part: %ld μs\n", avgFirstPart);
    printf("(*) Second part: %ld μs\n", avgSecondPart);
    printf("--------------------------------------------\n");

    free(firstPart);
    free(secondPart);
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
        int clientSocket, times_runned;
        char clientAddr[INET_ADDRSTRLEN];
        clock_t *firstPart = malloc(DEFAULT_SIZE_TIMES * sizeof(clock_t)), *secondPart = malloc(DEFAULT_SIZE_TIMES * sizeof(clock_t));
        int currentSize = DEFAULT_SIZE_TIMES;

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

        int totalReceived = 0, whichPart = 1;
        clock_t startTime;

        while (1)
        {
            int totalBytesReceived, buff = AUTH_CHECK;
            char buffer[FILE_SIZE/2];

            if (totalReceived == 0)
            {
                memset(buffer, 0, sizeof(buffer));
                printf_time("Waiting for client data...\n");
                startTime = clock();
            }

            totalBytesReceived = getDataFromClient(clientSocket, buffer, (FILE_SIZE/2));
            totalReceived += totalBytesReceived;

            if (totalReceived == (FILE_SIZE/2))
            {
                if (whichPart == 2)
                {
                    secondPart[times_runned] = clock() - startTime;

                    if (++times_runned == currentSize)
                    {
                        currentSize *= 2;
                        firstPart = realloc(firstPart, (currentSize * sizeof(clock_t)));
                        secondPart = realloc(secondPart, (currentSize * sizeof(clock_t)));
                    }

                    whichPart = 1;
                    printf_time("Second part received.\n");
                }

                else
                {
                    firstPart[times_runned] = clock() - startTime;
                    whichPart = 2;

                    printf_time("First part received, waiting for second part...\n");
                }

                printf_time("Received total %d bytes\n", totalReceived);
                printf_time("Sending authentication check...\n");
                send(clientSocket, &buff, sizeof(int), 0);
                printf_time("Authentication sent.\n");

                totalReceived = 0;
            }

            else if (buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't')
            {
                printf_time("Exit command received, exiting...\n");
                running = 0;
                break;
            }
        }

        calculateTimes(firstPart, secondPart, times_runned);

        close(clientSocket);
        printf_time("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);
    }

    usleep(1000*100);
    close(socketfd);
    printf_time("Server shutdown...\n");
    
    return 0;
}