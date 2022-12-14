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
#include <stdbool.h>
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

#define printf_time(f_, ...) printf("%s ", timestamp()), printf((f_), ##__VA_ARGS__);

int main() {
    struct sockaddr_in serverAddress, clientAddress;
    struct timeval tv_start, tv_end;

    socklen_t clientAddressLen;

    char clientAddr[INET_ADDRSTRLEN];

    int fileSize, times_runned = 0 ,totalReceived = 0, currentSize = 5, socketfd = INVALID_SOCKET, clientSocket = INVALID_SOCKET;

    bool whichPart = false;

    char* buffer = NULL;

    double *firstPart = (double*) malloc(currentSize * sizeof(double)), *secondPart = (double*) malloc(currentSize * sizeof(double));

    if (firstPart == NULL || secondPart == NULL)
    {
        perror("malloc");
        exit(1);
    }

    printf_time("Server starting up...\n");

    socketfd = socketSetup(&serverAddress);

    printf_time("Listening on %s:%d...\n", SERVER_IP_ADDRESS, SERVER_PORT);

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

    printf_time("Getting file size...\n");

    getDataFromClient(clientSocket, &fileSize, sizeof(int));
    syncTimes(clientSocket, true);

    printf_time("Expected file size is %d bytes (%.02f KiB / %.02f MiB).\n", fileSize, ((double)fileSize / 1024), ((double)fileSize/ 1048576));

    buffer = malloc(fileSize * sizeof(char));

    if (buffer == NULL)
    {
        perror("malloc");
        exit(1);
    }

    while (true)
    {
        int BytesReceived;
        
        if (!totalReceived)
        {
            if (!whichPart)
                memset(buffer, 0, fileSize);

            printf_time("Waiting for sender data...\n");
            gettimeofday(&tv_start, NULL);
        }

        BytesReceived = getDataFromClient(clientSocket, buffer + (totalReceived + (whichPart ? ((fileSize/2) - 1):0)), (fileSize/2) - totalReceived);
        totalReceived += BytesReceived;

        if (!BytesReceived)
            break;

        else if (totalReceived == (fileSize/2))
        {
            if (!whichPart)
            {
                gettimeofday(&tv_end, NULL);
                firstPart[times_runned] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);

                printf_time("Received total %d bytes.\n", totalReceived);
                printf_time("First part received.\n");

                authCheck(clientSocket);
            }

            else
            {
                gettimeofday(&tv_end, NULL);
                secondPart[times_runned] = ((tv_end.tv_sec - tv_start.tv_sec)*1000) + (((double)(tv_end.tv_usec - tv_start.tv_usec))/1000);

                if (++times_runned >= currentSize)
                {
                    currentSize += 5;

                    double* p1 = (double*) realloc(firstPart, (currentSize * sizeof(double)));
                    double* p2 = (double*) realloc(secondPart, (currentSize * sizeof(double)));

                    // Safe fail so we don't lose the pointers
                    if (p1 == NULL || p2 == NULL)
                    {
                        perror("realloc");
                        exit(1);
                    }

                    firstPart = p1;
                    secondPart = p2;
                }

                printf_time("Received total %d bytes.\n", totalReceived);
                printf_time("Second part received.\n");

                syncTimes(clientSocket, true);

                printf_time("Waiting for sender decision...\n");
                    
                if (!syncTimes(clientSocket, false))
                    break;

                syncTimes(clientSocket, true);
            }

            whichPart = (whichPart ? false:true);

            changeCCAlgorithm(socketfd, ((whichPart) ? 1:0));

            totalReceived = 0;
        }
    }

    close(clientSocket);
    printf_time("Connection with {%s:%d} closed.\n", clientAddr, clientAddress.sin_port);

    calculateTimes(firstPart, secondPart, times_runned, fileSize);

    usleep(1000);

    printf_time("Closing socket...\n");
    close(socketfd);

    printf_time("Memory cleanup...\n");
    free(firstPart);
    free(secondPart);
    free(buffer);

    printf_time("Receiver exit.\n");
    
    return 0;
}

/*
 * Function:  timestamp
 * --------------------
 * A fancy time formatation
 *
 *  returns: a pointer (basically a string)
 */
char* timestamp() {
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

    changeCCAlgorithm(socketfd, 0);

    return socketfd;
}

/*
 * Function:  getDataFromClient
 * --------------------
 * Get data from sender.
 *
 *  clientSocket: client's sock file descriptor.
 * 
 *  buffer: the buffer of data.
 * 
 *  len: buffer size.
 *
 *  returns: total bytes received,
 *           exit error 1 on fail.
 */
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
        printf_time("Sender doesn't accept requests.\n");
    }

    else if (sentd < len)
    {
        printf_time("Data was only partly send (%d/%d bytes).\n", sentd, len);
    }

    else
    {
        printf_time("Total bytes sent is %d.\n", sentd);
    }

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

/*
 * Function:  syncTimes
 * --------------------
 * A function that helps with timers and packet retransmission synchronization.
 *
 *  socketfd: socket file descriptor
 *  
 *  loss: are we in loss mode or timer sync mode.
 *
 *  returns: total bytes sent (always 1) or received 
 *           (1 for normal operation, 0 if the socket was closed),
 *           exit error 1 on fail.
 */
int syncTimes(int socketfd, bool loss) {
    static char dummy = 0;
    static int ret = 0;

    if (loss)
    {
        ret = send(socketfd, &dummy, sizeof(char), 0);

        if (ret == -1)
        {
            perror("send");
            exit(1);
        }

        return ret;
    }

    printf_time("Waiting for OK signal from sender...\n");
    ret = recv(socketfd, &dummy, sizeof(char), 0);

    if (ret == -1)
    {
        perror("recv");
        exit(1);
    }

    printf_time("OK – Continue\n");

    return ret;
}

/*
 * Function:  authCheck
 * --------------------
 * Makes an authentication check with the sender.
 *
 *  socketfd: socket file descriptor.
 *
 *  returns: 1 always
 */
int authCheck(int clientSocket) {
    char auth[5];

    printf_time("Sending authentication...\n");
    sprintf(auth, "%d", (ID1^ID2));
    sendData(clientSocket, &auth, sizeof(auth));
    printf_time("Authentication sent.\n");

    return 1;
}

/*
 * Function:  calculateTimes
 * --------------------
 * Makes an authentication check with the sender.
 *
 *  firstPart: a pointer that holds the array of all the receive
 *                  time of the first part of the file.
 * 
 *  secondPart: a pointer that holds the array of all the receive
 *                  time of the second part of the file.
 * 
 *  times_runned: number of times we sent the file.
 * 
 *  fileSize: size in bytes of the file that was sent.
 */
void calculateTimes(double* firstPart, double* secondPart, int times_runned, int fileSize) {
    double sumFirstPart = 0, sumSecondPart = 0, avgFirstPart = 0, avgSecondPart = 0;
    double bitrate;

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

    bitrate = ((fileSize / 1024) / ((avgFirstPart + avgSecondPart) / 1000));

    printf("\n(*) Time avarages:\n");
    printf("(*) First part (Cubic CC): %0.3lf ms\n", avgFirstPart);
    printf("(*) Second part (Reno CC): %0.3lf ms\n", avgSecondPart);
    printf("(*) Avarage transfer time for whole file: %0.3lf ms\n", (avgFirstPart+avgSecondPart));
    printf("(*) Avarage transfer bitrate: %0.3lf KiB/s (%0.3lf MiB/s)\n", bitrate, (bitrate / 1024));
    printf("--------------------------------------------\n");
}