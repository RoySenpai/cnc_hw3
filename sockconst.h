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

/* Wait time in microseconds. */
#define WAIT_TIME 1000

#endif