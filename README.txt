| Computer Networks Assignment- Q2	|
| Submitted by: 					|
| Tussank Gupta					 	|
| 2016B3A70528P					 	|
-------------------------------------

-----
Files
-----
srClient.c
srServer.c
srRelay.c
utils.c
utils.h
packet.h
config.h
makefile
README.txt
client (executable binary)
server (executable binary)
relay (executable binary)

---------
Execution
---------

Step 1: make
(This will compile and generate executables for client, server and relay)

Step 2: ./server output.txt
('output.txt' here is the name of the output file which is taken as a command line argument)

Step 3: ./relay 1
('1' here is the relay node number. There are only two valid choices, 1 and 2.)

Step 4: ./relay 2

Step 5: ./client input.txt
('input.txt' here is the name of the input file which is taken as a command line argument)

Input file name and output file name is given as command line argument to client and server respectively as described in above steps. All the nodes print their respective logs to the console and a sorted log file is generated in the end by the client and is stored by default with the name 'logfile.txt'.

----------------------
DEFAULT SPECIFICATIONS
----------------------
These default settings can easily be modified in the file 'config.h'. You need to use 'make' after making any changes to this config file.

1. PACKET_SIZE = 100
(Number of bytes of payload in a packet)
2. TIMEOUT_MILLISECONDS = 2000
(The time after which a packet is retransmitted if ACK is not received. Please put this time in milliseconds.)
3. PACKET_DROP_RATE = 10
(The percentage of packets which are randomly dropped by the server. Put a value between 0 and 100)
4. WINDOW_SIZE = 10
(This is the window size used in the selective repeat protocol. This affects the number of unack'd packets at a time for the client and this also affects the size of the buffer in the server.)

-----------------
LOG FILE DEFAULTS
-----------------
These default settings can easily be modified in the file 'config.h'. You need to use 'make' after making any changes to this config file.

1. LOG_FILE_NAME = "logfile.txt"
(Final logfile will be stored at this location.)
2. TMP_CLIENT_LOG = "log_client.txt"
3. TMP_SERVER_LOG = "log_server.txt"
4. TMP_RELAY_1_LOG = "log_relay1.txt"
5. TMP_RELAY_2_LOG = "log_relay2.txt"
(Above temporary files in 2-5 are generated during processing and are automatically deleted after processing.)

If all applications are accessed from different directories, it should be ensured that the temporary files are given absolute paths, otherwise the client won't be able to access them for preparing the final sorted log file.

The sorted logfile generation cannot work if all 4 applications are executed in different machines as the files are combined in the client program and should be accessible to the client locally.

---------------------
DEFAULT CONFIGURATION
---------------------
These default settings can easily be modified in the file 'config.h'. You need to use 'make' after making any changes to this config file.

#define RELAY_NODE_1_IP "127.0.0.1"
#define RELAY_NODE_1_PORT 9001
#define RELAY_NODE_2_IP "127.0.0.1"
#define RELAY_NODE_2_PORT 9002
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8089
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 8082

---------------------
IMPORTANT ASSUMPTIONS
---------------------

1. Buffer size is limited and is always equal to the product of PACKET_SIZE and WINDOW_SIZE. This will be always sufficient as the maximum number of unacknowledged packets in selective repeat protocol cannot exceed the window size.
2. It is assumed that the TIMEOUT_MILLISECONDS (which can be modified in 'config.h') will always be more the worst case round trip time for any packet.
3. The relay nodes are assumed to be independent of client and server. Their aim is only to add delays/drop packets and then forward them. So, they do not stop automatically after packet transmission, they can be stopped anytime by giving the SIGINT signal (CTRL + C). It will automatically close all sockets and exit.
4. It is assumed that all executables are executed from the same location, if not, then it is required by the user to change the paths for temporary log files in 'config.h' to absolute paths, so that these files can be accessed by the client application for combining the logs later to make a sorted list. It is therefore also assumed that all the applications run on the same machine for the purpose of combining logs only. 

-----------
METHODOLOGY
-----------

Sequence numbers are kept as integers in the range [0,(2*WINDOW_SIZE)-1], so that there can never be two packets of same sequence numbers in one window. 

Client, Server, Relay1 and Relay2 each create one UDP socket to handle UDP datagrams from both sides. 

To handle timeouts and retransmission, the client keeps an array of packets of size equal to its WINDOW_SIZE. Whenever a packet is first sent, a copy of it is kept in this array at the index equal to (pkt.seq)%WINDOW_SIZE. The send time is also noted in a parallel array of timevals at the same index. In every iteration, The remaining timeout value is checked for each sent packet in the window and the minimum of this is given in select call's timeout argument. During this, any timed out packet is retransmitted and its sent time is reset with the current time. 

In normal cases, select returns when one of the socket is available for reading, ACK Packet is read, and last sent for that channel is reset. Then, a new data packet is prepared (if there are more bytes in the input file) and sent over the same channel.

To drop a packet, a relay node generates a random integer in the range [0,100) and drops the packet if this random integer is less than or equal to the PACKET_DROP_RATE.

A relay node keeps an array of size proportional to the WINDOW_SIZE and keeps a copy of packet, its receive time and the random delay given to it. Just like the client handles each packet's timeout, here also, relay checks the remaining time for each packet in the window size array and forward packets accordingly. ACK packets are forwarded as soon as they are received.  

The termination condition for the client program is achieved when there are no more packets to be sent and sendBase is equal to the next sequence number obtained after the last packet.

The server is terminated once the last packet of the input file is written to the file.

Maintaing the Buffer:
Buffer is a character array which stores payload from packets and its size is always equal to the product of PACKET_SIZE and WINDOW_SIZE. However large may be the input file, this size will be sufficient as the number of unacknowledged packets in the network is upper bounded by the WINDOW_SIZE.
For ease in handling the buffer, A 2-dimension character array of dimensions WINDOW_SIZE * PACKET_SIZE is used, which is essentially same as a character array of WINDOW_SIZE * PACKET_SIZE. Payload from any out-of-order packet is copied at the index (pkt.seq)%WINDOW_SIZE. Whenever a in-sequence packet is obtained, all in-sequence payloads are copied to the file.

Logging the events:
Each one of client, server, relay1, relay2 logs their events in a temporary log file during their execution. The client program combines all these logs in the end to obtain the sorted log file.