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
#include <stdbool.h>
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

int main() {
    char *fileContent = NULL;
    int socketfd = INVALID_SOCKET, fileSize = 0, choice = 0;
    struct sockaddr_in serverAddress;

    printf("Client startup\n");

    printf("Reading file content...\n");
    fileContent = readFromFile(&fileSize);

    printf("Setting up the socket...\n");
    socketfd = socketSetup(&serverAddress);

    printf("Connection to %s:%d...\n",SERVER_IP_ADDRESS,SERVER_PORT);

    if (connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("Connected successfully to %s:%d!\n", SERVER_IP_ADDRESS, SERVER_PORT);

    printf("Sending file size to receiver...\n");

    sendData(socketfd, &fileSize, sizeof(int));
    syncTimes(socketfd, false);

    printf("File size sent successfully.\n");

    while(true)
    {
        printf("Sending the first part...\n");

        sendData(socketfd, fileContent, (fileSize/2));
        authCheck(socketfd);

        changeCCAlgorithm(socketfd, 1);

        printf("Sending the second part...\n");

        sendData(socketfd, (fileContent + (fileSize/2) - 1), (fileSize/2));
        syncTimes(socketfd, false);

        printf("Send the file again? (For data gathering)\n");
        printf("1 to resend, 0 to exit.\n");

        while (scanf("%d", &choice) != 1 || (choice != 1 && choice != 0));
        
        syncTimes(socketfd, true);
        syncTimes(socketfd, false);

        if (!choice)
        {
            printf("Exiting...\n");
            break;
        }

        changeCCAlgorithm(socketfd, 0);
    }

    printf("Closing connection...\n");

    close(socketfd);

    printf("Memory cleanup...\n");
    free(fileContent);

    printf("Sender exit.\n");

    return 0;
}

/*
 * Function:  socketSetup
 * --------------------
 * Setup the socket itself (convert the ip to binary and setup some settings).
 *
 *  serverAddress: a sockaddr_in struct that contains all the infromation
 *                  needed to connect to the receiver end.
 *
 *  returns: socket file descriptor if successed,
 *           exit error 1 on fail.
 */
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

    printf("Socket successfully created.\n");

    changeCCAlgorithm(socketfd, 0);

    return socketfd;
}

/*
 * Function:  readFromFile
 * --------------------
 *  A way to read the file content and put it into an array.
 *
 *  fileContent: a pointer to char array that will contain
 *                  the file content.
 */
char* readFromFile(int* size) {
    FILE * fpointer = NULL;
    char* fileContent;

    fpointer = fopen(fileName, "r");

    if (fpointer == NULL)
    {
        perror("fopen");
        exit(1);
    }

    // Find the file size and allocate enough memory for it.
    fseek(fpointer, 0L, SEEK_END);
    *size = (int) ftell(fpointer);
    fileContent = (char*) malloc(*size * sizeof(char));
    fseek(fpointer, 0L, SEEK_SET);

    fread(fileContent, sizeof(char), *size, fpointer);
    fclose(fpointer);

    printf("File \"%s\" total size is %d bytes (%.02f KiB / %.02f MiB).\n", fileName, *size, ((double)*size / 1024), ((double)*size / 1048576));

    return fileContent;
}

/*
 * Function:  sendData
 * --------------------
 * Sends data to receiver.
 *
 *  socketfd: socket file descriptor.
 * 
 *  buffer: the buffer of data.
 * 
 *  len: buffer size.
 *
 *  returns: total bytes sent,
 *           exit error 1 on fail.
 */
int sendData(int socketfd, void* buffer, int len) {
    int sentd = send(socketfd, buffer, len, 0);

    if (sentd == -1)
    {
        perror("send");
        exit(1);
    }

    else if (!sentd)
        printf("Receiver doesn't accept requests.\n");

    else if (sentd < len)
        printf("Data was only partly send (%d/%d bytes).\n", sentd, len);

    else
        printf("Total bytes sent is %d.\n", sentd);

    return sentd;
}

/*
 * Function:  authCheck
 * --------------------
 *  Makes an authentication check with the receiver.
 *
 *  socketfd: socket file descriptor.
 *
 *  returns: 1 on success,
 *           0 on fail.
 */
int authCheck(int socketfd) {
    char buffer[5], check[5];

    sprintf(check, "%d", (ID1^ID2));
    
    printf("Waiting for authentication...\n");
    recv(socketfd, &buffer, sizeof(buffer), 0);

    if (strcmp(buffer, check) != 0)
    {
        printf("Error with authentication!\n");
        return 0;
    }

    printf("Authentication OK.\n");

    return 1;
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

    printf("Waiting for OK signal from receiver...\n");
    ret = recv(socketfd, &dummy, sizeof(char), 0);

    if (ret == -1)
    {
        perror("recv");
        exit(1);
    }

    printf("OK – Continue\n");

    return ret;
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