#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include <stdint.h>

#include "util.h"
#include "mat.h"

char flags = 0;

char * zundo( char * compress , unsigned int size ){
	int ret;
	z_stream strm;
	char * uncompress = malloc( ZCHUNK );

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	inflateInit(&strm);

	strm.avail_in  = size;
	strm.next_in   = compress;
	strm.avail_out = ZCHUNK;
	strm.next_out  = uncompress;

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
	//		"\n\tavail_in: %i, next_in: %p, total_in: %i,"
	//		"\n\tavail_out: %i, next_out: %p, total_out: %i\n"
	//	"}\n",
	//	strm.avail_in, strm.next_in, strm.total_in,
	//	strm.avail_out, strm.next_out, strm.total_out
	//);

	if( flags & 0x01 ) for( int i = 0; i < strm.total_out; i++)
		putchar( uncompress[i] );
	
	if( ZCHUNK > strm.total_out )
		uncompress = realloc( uncompress, strm.total_out );
	
	inflateEnd(&strm);

	return uncompress;
}

void printWeightMatrix( WeightMatrix matrix ){
	printf("Edges: %i\tNodes: %i\n", matrix.edges, matrix.nodes );
	for( unsigned int n=0; n<matrix.edges; n++ )
		printf("%i\t%i\t%f\n", matrix.i[n], matrix.j[n], matrix.w[n] );
}

void printMatrix( Matrix matrix ){
	printf("%s{%i}( ", matrix.name, matrix.type );
	int size = 1;
	for( int i=0; i<matrix.dims_size; i++ ){
		printf("%i%c ", matrix.dims[i], i == matrix.dims_size-1 ? 0 : ',' );
		size *= matrix.dims[i];
	}
	printf(")[ ");
	for( int i=0; i<size; i++ )
		printf("%i%c ", matrix.data[i], i == size-1 ? 0 : ',' );
	printf("]\n");
}

Matrix * initMatrix( char * buffer ){
	int * p = (int*) buffer;
	Matrix * matrix = malloc( sizeof( Matrix ) );
	if(*p != 14) die("Matlab matrix was expected");
	p+=2;
	if(*p != 6 ) die("A uint32 flags was expected");
	p+=2;
	if(*p & 0x00000A00 ) die("This program is not desinged for complex numbers or logic matrix");
	if((*p & 0x000000FF)  < 6 ) die("A number class was expected");
	if((*p & 0x000000FF) != 6 ) warn("Data is expected to be double32, be careful");
	p+=2;
	if(*p != 5 ) die("A int32 dimensions array was expected");
	p++;
	matrix->dims_size = *p >> 2;
	matrix->dims = malloc( *p );
	memcpy( (char*) matrix->dims, (char*)(p+1), *p ); 
	p+=1+(*p>>2);
	if( *p&0xFFFF0000 ){
		short size = (*p & 0xFFFF0000) >> 16;
		short type = *p & 0x0000FFFF;
		if( type != miINT8 ) die("Char array was expected (Compressed data) got %p on byte %p", type, (char*) p - buffer);
		matrix->name = malloc( size + 1 );
		memcpy( matrix->name, (char*)++p, size ); 
		matrix->name[size] = 0;
		p+= (size>>2);
	} else {
		if( *p != miINT8 ) die("Char array was expected got %p on byte %p", *p, (char*)p - buffer);
		p++;
		matrix->name = malloc( *p + 1);
		memcpy( matrix->name, (char*)(p+1), *p );
		matrix->name[*p] = 0;
		p += 1 + (*p>>2);
	}
	if( *p&0xFFFF0000 ){
		short size = *p & 0xFFFF0000 >> 16;
		short type = *p & 0x0000FFFF;
		if( type > miUINT64 || type == 8 || type == 10 || type == 11 )
			die("Number array was expected (Compressed) got %p on byte %p",type, (char*)p - buffer );
		matrix->type = type;
		matrix->data = malloc( size );
		memcpy( matrix->data, (char*)++p, size );
		p+= (size>>2);
	}else{
		if( *p > miUINT64 || *p == 8 || *p == 10 || *p == 11 )
			die("Number array was expected got %p on byte %p", *p, (char*)p - buffer );
		matrix->type = *p;
		p++;
		matrix->data = malloc( *p );
		memcpy( matrix->data, (char*)(p+1), *p );
		p += 1 + (*p>>2);
	}
	free( buffer );
	return matrix;
}

void readHeader( int fd ){
	if( ! ( flags & 0x08 ) ) {
		lseek( fd, 128, SEEK_CUR );
		return;
	}
	char * buffer = malloc( 116 );

	read( fd, buffer, 116 );
	fprintf(stderr, "%.116s\n", buffer );
	free( buffer );

	long int offset = 0;
	read( fd, & offset, 8 );
	if( offset ) fprintf(stderr, "%li offset bytes to subsystem-specific info\n", * ( long int * ) buffer );

	int version_im = 0;
	read( fd, & version_im, 4 );
	fprintf(stderr, "Mat file version: %hu\tEndian indicator %c%c %s\n",
		(version_im&0x0000FFFF)>>0,
		(version_im&0xFF000000)>>24,
		(version_im&0x00FF0000)>>16,
		(version_im&0xFF000000)>>24 == 'I' ? "(Must reverse)" : ""
	);
}

Matrix * readMatrix( int fd ){
	unsigned int type, size;
	read( fd, &type, 4 );
	read( fd, &size, 4 );
	if( type != 14 && type != 15 ) die("Expected a miMatrix or miCompressed with a miMatrix");
	char * buffer = malloc( size );
	read( fd, buffer, size );
	Matrix * matrix = malloc( sizeof(Matrix) );
	if( type == 14 )
		matrix = initMatrix( buffer );
	else if( type == 15 )
		matrix = initMatrix( zundo( buffer, size ) );
	if( flags & 0x02 )
		printMatrix( * matrix );
	return matrix;
}

WeightMatrix * initWeightMatrix( unsigned int nodes, unsigned edges, Matrix * matrix){
	if( matrix->dims_size > 2 )
		die("2D Weight matrix was expected");
	if( matrix->dims[1] != 3 )
		die("Weight matrix must have exactly 3 colums");
	WeightMatrix * wm = malloc( sizeof( WeightMatrix ) );
	wm->nodes = nodes;
	wm->edges = edges;
	wm->i = malloc( sizeof(int)    * matrix->dims[0] );
	wm->j = malloc( sizeof(int)    * matrix->dims[0] );
	wm->w = malloc( sizeof(double) * matrix->dims[0] );
	switch( matrix->type ){
		case miINT8:  case miUINT8: 
			for( unsigned int n=0; n<matrix->dims[0];n++){
				wm->i[n] = ((char*)matrix->data)[matrix->dims[0]*0+n];
				wm->j[n] = ((char*)matrix->data)[matrix->dims[0]*1+n];
				wm->w[n] = ((char*)matrix->data)[matrix->dims[0]*2+n];
			}
		break;
		case miINT16:  case miUINT16: 
			for( unsigned int n=0; n<matrix->dims[0];n++){
				wm->i[n] = ((short*)matrix->data)[matrix->dims[0]*0+n];
				wm->j[n] = ((short*)matrix->data)[matrix->dims[0]*1+n];
				wm->w[n] = ((short*)matrix->data)[matrix->dims[0]*2+n];
			}
		break;
		case miINT32:  case miUINT32: 
			for( unsigned int n=0; n<matrix->dims[0];n++){
				wm->i[n] = ((int*)matrix->data)[matrix->dims[0]*0+n];
				wm->j[n] = ((int*)matrix->data)[matrix->dims[0]*1+n];
				wm->w[n] = ((int*)matrix->data)[matrix->dims[0]*2+n];
			}
		break;
		default: die("miMAT-Data not supported");
	}
	//printWeightMatrix( * wm );
	return wm;
}

void mimatrix( char * buffer, ssize_t size ){
	int type = *(int*)buffer;
	printf("\tType: %s, ", mat_data_string[type] );
	buffer+=4;
	type = *(int*) buffer ;
	printf("size: %u\n", type );
	buffer+=4;
	char flags = buffer[1];
	char class = buffer[0];
	printf("\tFlags: COMPLEX: %hhu, GLOBAL: %hhu, LOGICAL: %hhu, HEXA: %02X\n", flags & 0x10, flags & 0x20, flags & 0x40, flags );
	printf("\tClass: %s\n", mat_array_string[class]);
	buffer += 4;
	int max_sparce = *(int*) buffer;
	printf("\tMax zeros: %u\n", max_sparce );
}

void run( char * path ){
	int fd = open( path, O_RDONLY );
	if( fd < 0 ) die("fail on open %s:", path );

	char * header = malloc( 116 );
	read( fd, header, 116 );
	printf("%.116s\n", header );
	uint64_t off_sub_sys = 0;
	read( fd, & off_sub_sys, 8 );
	if( off_sub_sys ) printf("Offset to sub-system info: %lu\n", off_sub_sys );
	uint16_t version;
	read( fd, & version, 2 );
	printf("Version: %hu\n", version );
	char endian[2];
	read( fd, & endian, 2 );
	printf("Endian %.2s\n", endian );
	uint32_t data_type, num_bytes;
	while( read( fd, & data_type, 4 ) &&  read( fd, & num_bytes, 4 ) ){
		if( data_type == miCOMPRESSED ){
			char * buffer = malloc( num_bytes );
			read( fd, buffer, num_bytes );
			char * buffer2 = zundo( buffer, num_bytes );
			printf("Data type: %s, size: %u\n", mat_data_string[ * (int*) buffer2 ], *(int*) (buffer+4) );
			if( * (int*) buffer2 == miMATRIX ) mimatrix( buffer2 + 8, *(int*) (buffer+4) );
			free( buffer );
			free( buffer2 );
		} else {
			printf("Data type: %s, size: %u\n", mat_data_string[data_type], num_bytes );
			lseek( fd, num_bytes, SEEK_CUR );
		}
	}
}

void usage(){
	printf("matio <file>...\n");
	exit(0);
}


int main(int argc, char ** argv){
	for( uint32_t i=1; i<argc; i++){
		if( ! strcmp( argv[i], "-h" ) ) usage();
		else run( argv[i] );
	}
	return 0;
}
