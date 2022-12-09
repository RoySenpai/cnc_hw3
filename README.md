# Communication and Computing Course Assigment 3
### For Computer Science B.Sc Ariel University

**By Roy Simanovich and Yuval Yurzdichinsky**

## Description
In this Ex we wrote two program files: Sender.c and Receiver.c. The Sender will send
a file (which is at least 1MB) and the Receiver will receive it and measure the time
it took for his program to receive the file. The file will be sent in
two parts (first half and second half – Each half should be 50% of the file).
Each half will be sent using reno and cubic CC algorithms respectively.
The Sender can send the file multiple times, to create a data set of the efficiency of each of the CC
algorithms and the efficiency of the pipeline itself.
To see how the TCP protocol handles the packet loss with each CC algorithm, we use the packet lost tool
that linux have via the iproute package. In this Ex we will use the
packet lost tool in the following levels: 0% lost, 10% lost, 15% lost and 20% lost.

# Requirements
* Linux machine (Virtual or real hardware)
* GNU C Compiler
* Make
* iproute (Needs a full real hardware/virtualized environment)

## Building
```
# Cloning the repo to local machine
git clone https://github.com/RoySenpai/cnc_hw3.git

# Building all the necessary files & the main programs
make all
```

## Running
```
# Runs the server ("Receiver" part) program.
./Receiver

# Runs the client ("Sender" part) program.
./Sender
```

## Packet-loss tool
```
# Setting up a packet-loss rate to XX% (from 0% to 100%).
sudo tc qdisc add dev lo root netem loss XX%

# Change the packet-loss rate to XX% (from 0% to 100%).
sudo tc qdisc change dev lo root netem loss XX%

# Disable the packet-loss tool.
sudo tc qdisc del dev lo root netem
```
