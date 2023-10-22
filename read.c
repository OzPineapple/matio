#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include <stdint.h>

#include "util.h"
#include "mat.h"

char * zundo( char * compress , unsigned int * size ){
	int ret;
	z_stream strm;
	char * uncompress = malloc( ZCHUNK );

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	inflateInit(&strm);

	strm.avail_in  = * size;
	strm.next_in   = (unsigned char*) compress;
	strm.avail_out = ZCHUNK;
	strm.next_out  = (unsigned char*) uncompress;

	//fprintf(stderr,"zlib init with a %i buffer byte long\n", ZCHUNK );

	//fprintf(stderr,"strm: {" 
	//		"\n\tavail_in: %i, next_in: %p, total_in: %i,"
	//		"\n\tavail_out: %i, next_out: %p, total_out: %i\n"
	//	"}\n",
	//	strm.avail_in, strm.next_in, strm.total_in,
	//	strm.avail_out, strm.next_out, strm.total_out
	//);

	ret = inflate( &strm, Z_NO_FLUSH );

	switch( ret ){
		case Z_STREAM_ERROR:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			die("uncompress data falied:");
	}

	//fprintf(stderr,"strm: {" 
	//		"\n\tavail_in: %u, next_in: %p, total_in: %lu,"
	//		"\n\tavail_out: %u, next_out: %p, total_out: %lu\n"
	//	"}\n",
	//	strm.avail_in, strm.next_in, strm.total_in,
	//	strm.avail_out, strm.next_out, strm.total_out
	//);

	if( 0 ) for( int i = 0; i < strm.total_out; i++)
		putchar( uncompress[i] );
	
	if( ZCHUNK > strm.total_out )
		uncompress = realloc( uncompress, strm.total_out );
	
	* size = strm.total_out;

	inflateEnd(&strm);

	return uncompress;
}

typedef struct {
	uint8_t  text[116];
	uint64_t subsystem_data_offset;
	uint16_t version;
	uint8_t  endian[2];
} MATheader;

typedef struct {
	uint8_t  complex:1;
	uint8_t  global:1;
	uint8_t  logical:1;
	uint32_t sparce_ceros;
	uint32_t class;
	uint32_t dimentions;
	int32_t  * shape;
	uint8_t  * name;
	uint32_t data_lenght;
	uint32_t * data;
} miMatrix;

void readMATtag( char ** buffer, uint32_t * data_type, uint32_t * size ){
	* data_type = * (uint32_t*) (*buffer);
	if( (* data_type) & 0xFFFF0000 ){
		* size      = (* data_type & 0xFFFF0000) >> 16;
		* data_type =  * data_type & 0x0000FFFF;
		* buffer = (*buffer)+4;
	} else {
		* size   = * (uint32_t*) ((*buffer)+4);
		* buffer = (*buffer)+8;
	}
	//printf("Data type: %s, size: %u\n", mat_data_string[*data_type], * size );
}

void printMATmiMatrix( miMatrix * matrix ){
	printf(mat_data_string[miMATRIX]);
	printf(" \"%s\"", matrix->name );
	if( matrix->complex )
		printf("complex ");
	if( matrix->global )
		printf("global ");
	if( matrix->logical )
		printf("logical ");
	printf(" %s (", mat_array_string[matrix->class] );
	for( ssize_t i=0; i<matrix->dimentions; i++)
		printf("%u%c", matrix->shape[i], i+1==matrix->dimentions ? ')' : ',' );
	printf("\n");
}

void * loadMATmiMatrix( char * buffer, uint32_t size ){
	miMatrix * matrix = malloc( sizeof( miMatrix ) );
	uint32_t type, bytes;

	readMATtag( &buffer, &type, &bytes);
	matrix->class    = buffer[0];
	matrix->complex  = buffer[1] & 0x10;
	matrix->global   = buffer[1] & 0x20;
	matrix->logical  = buffer[1] & 0x40;
	matrix->sparce_ceros = * (uint32_t*) (buffer+4);
	buffer += bytes + bytes%8?8-bytes%8:0;

	readMATtag( &buffer, &type, &bytes);
	matrix->dimentions = bytes / 4;
	matrix->shape = malloc( bytes );
	memcpy( (char*) matrix->shape, buffer, bytes );
	buffer += bytes + bytes%8?8-bytes%8:0;

	readMATtag( &buffer, &type, &bytes);
	matrix->name = malloc( bytes + 1 );
	matrix->name[bytes] = 0;
	memcpy( (char*) matrix->name, buffer, bytes );
	buffer += bytes + bytes%8?8-bytes%8:0;

	readMATtag( &buffer, &type, &bytes);
	return matrix;
}

void printMATheader( MATheader * header ){
	printf("%.116s.", header->text );
	if( header->subsystem_data_offset )
		printf(" Subsystem Data Offset: %lu.", header->subsystem_data_offset );
	printf(" Version: %hu. Endian indicator: %.2s.\n", header->version, header->endian );
}

void skipMATheader( int fd ){ lseek( fd, 128, SEEK_CUR ); }

MATheader * readMATheader( int fd ){
	MATheader * header = malloc( sizeof(MATheader) );
	read( fd, & header->text,    116 );
	read( fd, & header->subsystem_data_offset, 8);
	read( fd, & header->version, 2   );
	read( fd, & header->endian,  2   );
	return header;
}

void * loadMAT( char * buffer, uint32_t data_type, uint32_t size ){
	switch( data_type ){
		case miMATRIX: return loadMATmiMatrix( buffer, size );
	}
	die("i will code this, I promise");
	return NULL;
}

void * uncompressMAT( int fd, uint32_t * data_type, uint32_t * size ){
	char * compressed = malloc( * size );
	read( fd, compressed, * size );
	char * uncompressed = zundo( compressed, size );
	readMATtag( & uncompressed, data_type, size );
	free( compressed );
	return loadMAT( uncompressed, * data_type, * size );
}

void * readMATnext( int fd, uint32_t * data_type, uint32_t * size ){
	if( ! read( fd, data_type, 4 ) ) return NULL;

	if( * data_type & 0xFFFF0000 ){
		* size      = (* data_type & 0xFFFF0000) >> 16;
		* data_type =  * data_type & 0x0000FFFF;
	} else read( fd, size, 4 );

	if( * data_type == miCOMPRESSED )
		return uncompressMAT( fd, data_type, size );

	char * buffer = malloc( * size );
	read( fd, buffer, * size );
	return loadMAT(buffer, * data_type, * size );
}

void run( char * path ){
	int fd = open( path, O_RDONLY );
	if( fd < 0 ) die("fail on open %s:", path );
	
	printMATheader( readMATheader( fd ) );

	uint32_t data_type, num_bytes;

	void * object;

	while( (object = readMATnext( fd, & data_type, & num_bytes )) ){
		if( data_type == miMATRIX )
			printMATmiMatrix( object );
	}
}

void usage(){
	printf("matio <file>...\n");
	exit(0);
}

int main(int argc, char ** argv){
	for( uint32_t i=1; i<argc; i++)
		if( ! strcmp( argv[i], "-h" ) ) usage();
		else run( argv[i] );
}
