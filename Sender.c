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
#include "sockconst.h"

char * fileName = "sendthis.txt";
char * CC_reno = "reno";
char * CC_cubic = "cubic";

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

int sendData(int socketfd, void* buffer, int len) {
    int sentd = send(socketfd, buffer, len, 0);

    if (sentd == 0)
        printf("Server doesn't accept requests.\n");

    else if (sentd < len)
    {
        printf("Data was only partly send (%d/%d bytes).\n", sentd, len);
    }

    else
        printf("Total bytes sent is %d.\n", sentd);

    usleep(WAIT_TIME);

    return sentd;
}

int authCheck(int socketfd) {
    int buffer = 0, check = AUTH_CHECK;
    
    recv(socketfd, &buffer, sizeof(int), 0);

    printf("Sending authentication...\n");

    sendData(socketfd, &check, sizeof(int));

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

void changeCCAlgorithm(int socketfd, int whichOne) {
    printf("Changing congestion control algorithm to %s...\n", (whichOne ? "reno":"cubic"));

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

int main() {
    char fileContent[FILE_SIZE];
    int socketfd = INVALID_SOCKET;
    struct sockaddr_in serverAddress;
    char* exitcmd = "exit";
    int exitcmdlen = (int) (strlen(exitcmd) + 1);

    printf("Client startup\n");

    printf("Reading file content...\n");
    readFromFile(fileContent);

    printf("Setting up the socket...\n");
    socketfd = socketSetup(&serverAddress);

    printf("Connection to %s:%d...\n",SERVER_IP_ADDRESS,SERVER_PORT);

    if (connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("Connected successfully to %s:%d!\n", SERVER_IP_ADDRESS, SERVER_PORT);

    while(1)
    {
        int choice;

        printf("Sending the first part...\n");

        sendData(socketfd, fileContent, (FILE_SIZE/2));
        authCheck(socketfd);

        changeCCAlgorithm(socketfd, 1);

        printf("Sending the second part...\n");

        sendData(socketfd, (fileContent+(FILE_SIZE/2)), (FILE_SIZE/2));

        printf("Send the file again? (For data gathering)\n");
        scanf("%d", &choice);

        if (!choice)
        {
            printf("Sending exit command to receiver.\n");
            sendData(socketfd, exitcmd, exitcmdlen);
            break;
        }

        changeCCAlgorithm(socketfd, 0);
    }

    printf("Closing connection...\n");

    close(socketfd);

    return 0;
}