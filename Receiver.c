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
#include <sys/time.h>
#include "sockconst.h"

char * CC_reno = "reno";
char * CC_cubic = "cubic";

char * timestamp();
int socketSetup(struct sockaddr_in*);
int getDataFromClient(int, void*, int);
int sendData(int, void*, int);
void changeCCAlgorithm(int, int);
void authCheck(int);
void calculateTimes(double*, double*, int);

#define printf_time(f_, ...) printf("%s ", timestamp()), printf((f_), ##__VA_ARGS__);

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
        double *firstPart = malloc(sizeof(double)), *secondPart = malloc(sizeof(double));
        int currentSize = 1;

        memset(&clientAddress, 0, sizeof(clientAddress));

        clientAddressLen = sizeof(clientAddress);

        clientSocket = accept(socketfd, (struct sockaddr *) &clientAddress, &clientAddressLen);

        if (clientSocket == -1)
        {
            perror("accept");
            exit(1);
        }
        
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET_ADDRSTRLEN);

        printf_time("Connection made with {%s:%d}.\n", clientAddr, clientAddress.sin_port);

        int totalReceived = 0, whichPart = 1;
        struct timeval tv_start;
        struct timeval tv_end;

        while (1)
        {
            int totalBytesReceived = 0;
            char buffer[FILE_SIZE/2];

            if (totalReceived == 0)
            {
                memset(buffer, 0, sizeof(buffer));
                printf_time("Waiting for client data...\n");
                //startTime = clock();
                gettimeofday(&tv_start, NULL);
            }

            totalBytesReceived = getDataFromClient(clientSocket, buffer, (FILE_SIZE/2));
            totalReceived += totalBytesReceived;

            if (totalBytesReceived == 0)
            {
                running = 0;
                break;
            }

            if (totalReceived == (FILE_SIZE/2) || totalReceived == (FILE_SIZE/2) - 3)
            {
                if (whichPart == 2)
                {
                    gettimeofday(&tv_end, NULL);
                    secondPart[times_runned] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);
                    if (++times_runned > currentSize)
                    {
                        currentSize *= 2;
                        firstPart = realloc(firstPart, (currentSize * sizeof(clock_t)));
                        secondPart = realloc(secondPart, (currentSize * sizeof(clock_t)));
                    }

                    whichPart = 1;
                    printf_time("Second part received.\n");
                    printf_time("Waiting for client command...\n");

                    char buff[8];
                    getDataFromClient(clientSocket, buff, sizeof(buff));

                    if (buff[0] == 'e' && buff[1] == 'x' && buff[2] == 'i' && buff[3] == 't')
                    {
                        printf_time("Exit command received, exiting...\n");
                        running = 0;
                        break;
                    }

                    else if (buff[0] == 'o' && buff[1] == 'k')
                    {
                        printf_time("OK command received, continueing...\n");
                    }
                }

                else
                {
                    gettimeofday(&tv_end, NULL);
                    firstPart[times_runned] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);
                    whichPart = 2;

                    printf_time("First part received.\n");

                    printf_time("Sending authentication...\n");

                    char auth[5];
                    sprintf(auth, "%d", (ID1^ID2));
                    sendData(clientSocket, &auth, sizeof(auth));
                    printf_time("Authentication sent.\n");
                }

                printf_time("Received total %d bytes.\n", totalReceived);

                changeCCAlgorithm(socketfd, ((whichPart == 1) ? 0:1));

                totalReceived = 0;
            }
        }

        calculateTimes(firstPart, secondPart, times_runned);

        close(clientSocket);
        printf_time("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);
    }

    usleep(WAIT_TIME);
    close(socketfd);
    printf_time("Server shutdown...\n");
    
    return 0;
}

char * timestamp() {
    char buffer[16];
    time_t now = time(NULL);
    struct tm* timest = localtime(&now);

    strftime(buffer, sizeof(buffer), "(%H:%M:%S)", timest);

    char * time = buffer;

    return time;
}

/*
 * Function:  socketSetup
 * --------------------
 * Setup the socket itself (bind, listen, etc.).
 *
 *  serverAddress: a sockaddr_in struct that contains all the infromation
 *                  needed to connect to the receiver end.
 *
 *  returns: socket file descriptor if successed,
 *           exit error 1 on fail.
 */
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

int getDataFromClient(int clientSocket, void *buffer, int len) {
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

/*
 * Function:  sendData
 * --------------------
 * Sends data to sender.
 *
 *  clientSocket: client's sock file descriptor.
 * 
 *  buffer: the buffer of data.
 * 
 *  len: buffer size.
 *
 *  returns: total bytes sent,
 *           exit error 1 on fail.
 */
int sendData(int clientSocket, void* buffer, int len) {
    int sentd = send(clientSocket, buffer, len, 0);

    if (sentd == 0)
    {
        printf_time("Client doesn't accept requests.\n");
    }

    else if (sentd < len)
    {
        printf_time("Data was only partly send (%d/%d bytes).\n", sentd, len);
    }

    else
    {
        printf_time("Total bytes sent is %d.\n", sentd);
    }

    usleep(WAIT_TIME);

    return sentd;
}

/*
 * Function:  changeCCAlgorithm
 * --------------------
 *  Change the TCP Congestion Control algorithm (reno or cubic). 
 *
 *  socketfd: socket file descriptor.
 * 
 *  whichOne: 1 for reno, 0 for cubic.
 */
void changeCCAlgorithm(int socketfd, int whichOne) {
    printf_time("Changing congestion control algorithm to %s...\n", (whichOne ? "reno":"cubic"));

    if (whichOne)
    {
        socklen_t CC_reno_len = strlen(CC_reno);

        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, CC_reno, CC_reno_len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }

    else
    {
        socklen_t CC_cubic_len = strlen(CC_cubic);

        if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, CC_cubic, CC_cubic_len) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
}

void calculateTimes(double * firstPart, double * secondPart, int times_runned) {
    double sumFirstPart = 0, sumSecondPart = 0, avgFirstPart = 0, avgSecondPart = 0;

    printf("--------------------------------------------\n");
    printf("(*) Times summary:\n\n");
    printf("(*) Cubic CC:\n");

    for (int i = 0; i < times_runned; ++i)
    {
        sumFirstPart += firstPart[i];
        printf("(*) Run %d, Time: %0.3lf ms\n", (i+1), firstPart[i]);
    }

    printf("\n(*) Reno CC:\n");

    for (int i = 0; i < times_runned; ++i)
    {
        sumSecondPart += secondPart[i];
        printf("(*) Run %d, Time: %0.3lf ms\n", (i+1), secondPart[i]);
    }

    if (times_runned > 0)
    {
        avgFirstPart = (sumFirstPart / (double)times_runned);
        avgSecondPart = (sumSecondPart / (double)times_runned);
    }

    printf("\n(*) Time avarages:\n");
    printf("(*) First part (Cubic CC): %0.3lf ms\n", avgFirstPart);
    printf("(*) Second part (Reno CC): %0.3lf ms\n", avgSecondPart);
    printf("--------------------------------------------\n");

    free(firstPart);
    free(secondPart);
}