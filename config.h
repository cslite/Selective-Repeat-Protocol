/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#ifndef CNP2_CONFIG_H
#define CNP2_CONFIG_H

//Packet Size in number of bytes
#define PACKET_SIZE 100
//Timeout in Milliseconds (for resending a packet)
#define TIMEOUT_MILLISECONDS 2000
//Rate for dropping data packets
#define PACKET_DROP_RATE 10 //in percentage (example 10 for 10%)

//Window Size for selective repeat protocol
#define WINDOW_SIZE 10

/*** IP & PORT CONFIGURATIONS ***/
#define RELAY_NODE_1_IP "127.0.0.1"
#define RELAY_NODE_1_PORT 9001
#define RELAY_NODE_2_IP "127.0.0.1"
#define RELAY_NODE_2_PORT 9002
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8089
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 8082

//Path for logfile
#define LOG_FILE_NAME "logfile.txt"


//Turn this on to print extra debug messages
#define DEBUG_MODE 0

//Path for temporary logfiles (Ensure that all these files get stored to same folder)
#define TMP_CLIENT_LOG "log_client.txt"
#define TMP_SERVER_LOG "log_server.txt"
#define TMP_RELAY_1_LOG "log_relay1.txt"
#define TMP_RELAY_2_LOG "log_relay2.txt"


#endif //CNP2_CONFIG_H
