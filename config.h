#ifndef CNP2_CONFIG_H
#define CNP2_CONFIG_H

//Packet Size in number of bytes
#define PACKET_SIZE 100
//Timeout in Milliseconds (for resending a packet)
#define TIMEOUT_MILLISECONDS 2000
//Rate for dropping data packets
#define PACKET_DROP_RATE 10 //in percentage (example 10 for 10%)

#define WINDOW_SIZE 10

#define RELAY_NODE_1_IP "127.0.0.1"
#define RELAY_NODE_1_PORT 9001
#define RELAY_NODE_2_IP "127.0.0.1"
#define RELAY_NODE_2_PORT 9002
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8089
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 8082

#define DEBUG_MODE 1

#endif //CNP2_CONFIG_H
