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

#ifndef _SOCKCONST_H
#define _SOCKCONST_H

/* Default value for invalid socket file descriptor. */
#define INVALID_SOCKET -1

/* Defines maximum senders that can simultaneously connect to the receiver. */
#define MAX_QUEUE 1

/* Receiver IP address */
#define SERVER_IP_ADDRESS "127.0.0.1"

/* Receiver port */
#define SERVER_PORT 8888

/* Authentication check (XOR) */
#define ID1 6159
#define ID2 2175

// Below are functions that are used for the receiver and the sender.

void calculateTimes(double*, double*, int, int);

void changeCCAlgorithm(int, int);

char * timestamp();

char* readFromFile(int*);

int socketSetup(struct sockaddr_in*);

int sendData(int, void*, int);

int authCheck(int);

int syncTimes(int, bool);

int getDataFromClient(int, void*, int);

int syncTimes(int, bool); 

#endif