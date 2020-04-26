#ifndef CNP2_UTILS_H
#define CNP2_UTILS_H


#include "packet.h"

uint mdval(uint seq);

uint mdadd(uint seq, uint val);

uint mdsub(uint seq, uint val);

/*
 * Returns the size of the given file
 * Returns -1 if the file can't be opened.
 */
int getFileSize(char *file);

int max(int x, int y);

void initPacket(packet *pkt);

#endif //CNP2_UTILS_H
