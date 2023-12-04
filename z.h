#ifndef ZHEADER
#define ZHEADER
#define ZCHUNK 1<<10
char * zundo( char * compress, unsigned int size );
char * zdo( char * uncompress, unsigned int size, unsigned int * out_size );
#endif
