#ifndef MATIO_H
#define MATIO_H
void readHeader( int fd );
void writeHeader( int fd );
char * loadNextMat( int fd, uint32_t * size );
#endif
