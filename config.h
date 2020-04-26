#ifndef CNP2_CONFIG_H
#define CNP2_CONFIG_H

//Packet Size in number of bytes
#define PACKET_SIZE 100
//Timeout in Milliseconds (for resending a packet)
#define TIMEOUT_MILLISECONDS 2000
//Rate for dropping data packets
#define PACKET_DROP_RATE 50 //in percentage (example 10 for 10%)

#define WINDOW_SIZE 10

#define RELAY_NODE_1_IP "127.0.0.1"
#define RELAY_NODE_1_DEFAULT_PORT 9003
#define RELAY_NODE_2_IP "127.0.0.1"
#define RELAY_NODE_2_DEFAULT_PORT 9003

#define DEBUG_MODE 1

#endif //CNP2_CONFIG_H
