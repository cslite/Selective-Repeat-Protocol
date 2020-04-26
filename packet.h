#ifndef CNP2_PACKET_H
#define CNP2_PACKET_H

#include "config.h"

typedef enum{
    DATA_PKT, ACK_PKT
} pktType;

struct packet{
    unsigned int size;  //size of the packet in bytes
    unsigned int seq;   //offset of the first byte of the packet
    bool isLastPkt;
    pktType ptype;
    char payload[PACKET_SIZE+1];
};
typedef struct packet packet;

#endif //CNP2_PACKET_H
