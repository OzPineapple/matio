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

void printMATmiMatrixHeader( miMatrixHeader * header ){
	printf(mat_data_string[miMATRIX]);
	printf(" \"%s\"", header->name );
	if( header->complex )
		printf("complex ");
	if( header->global )
		printf("global ");
	if( header->logical )
		printf("logical ");
	printf(" %s (", mat_array_string[header->class] );
	for( ssize_t i=0; i<header->dimentions; i++)
		printf("%u%c", header->shape[i], i+1==header->dimentions ? ')' : ',' );
	printf("\n");
}

void arraycpy( void * dest, enum mat_array_type dest_type , void * source, enum mat_data_type source_type, ssize_t length){
	#define FOR_TYPES( DEST_TYPE, SOURCE_TYPE ) \
		for( ssize_t i=0; i<length; i++ ) \
			((DEST_TYPE*) dest)[i] = (DEST_TYPE) ((SOURCE_TYPE*) source)[i];
	#define SWITCH_TYPE( DEST_TYPE, SOURCE_TYPE ) \
		switch( SOURCE_TYPE ){ \
			case miUINT8: \
			case miINT8: \
				FOR_TYPES( DEST_TYPE, int8_t )\
			break;\
			case miUINT16: \
			case miINT16: \
				FOR_TYPES( DEST_TYPE, int16_t )\
			break;\
			case miUINT32: \
			case miINT32: \
				FOR_TYPES( DEST_TYPE, int32_t )\
			break;\
			case miUINT64: \
			case miINT64: \
				FOR_TYPES( DEST_TYPE, int64_t ) \
			break;\
			case miSINGLE: \
				FOR_TYPES( DEST_TYPE, float ) \
			break;\
			case miDOUBLE: \
				FOR_TYPES( DEST_TYPE, double ) \
			break;\
			default: \
				die("Error: %s to %s.", mat_data_string[source_type], mat_array_string[dest_type]); \
		}
	switch( dest_type ){
		case mxUINT8_CLASS:
		case mxINT8_CLASS:
		        SWITCH_TYPE( int8_t, source_type )
		break;
		case mxUINT16_CLASS:
		case mxINT16_CLASS:
		        SWITCH_TYPE( int8_t, source_type )
		break;
		case mxUINT32_CLASS:
		case mxINT32_CLASS:
		        SWITCH_TYPE( int8_t, source_type )
		break;
		case mxUINT64_CLASS:
		case mxINT64_CLASS:
		        SWITCH_TYPE( int8_t, source_type )
		break;
		case mxSINGLE_CLASS:
		        SWITCH_TYPE( float, source_type )
		break;
		case mxDOUBLE_CLASS:
			SWITCH_TYPE( double, source_type )
		break;
		default:
			die("Error: %s to %s.", mat_data_string[source_type], mat_array_string[dest_type]);
	}
}

#define PLUS64( x ) x + ( x <= 4 ? 4 - x : x % 8 ? 8 - ( x % 8 ) : 0 )

void * loadMATmiNumericMatrix( char * buffer_src, uint32_t size, miMatrixHeader * header ){
	uint32_t type, bytes;
	char * buffer = buffer_src;
	miNumericMatrix * matrix = malloc( sizeof( miNumericMatrix ) );

	readMATtag( &buffer, &type, &bytes);
	ssize_t length = bytes / mat_data_size[ type ];
	matrix->real = malloc( mat_array_size[ header->class ] * length );
	arraycpy( matrix->real, header->class, buffer, type, length );
	buffer += PLUS64( bytes );

	if( header->complex ){
		readMATtag( &buffer, &type, &bytes);
		length = bytes / mat_data_size[ type ];
		matrix->imaginary = malloc( mat_array_size[ header->class ] * length );
		arraycpy( matrix->imaginary, header->class, buffer, type, length );
	} else matrix->imaginary = NULL;

	return matrix;
}

void * loadMATmiMatrix( char * buffer_src, uint32_t size ){
	char * buffer = buffer_src;
	miMatrix * matrix = malloc( sizeof( miMatrix ) );
	miMatrixHeader * header = malloc( sizeof( miMatrixHeader ) );
	matrix->header = header;
	uint32_t type, bytes;

	readMATtag( &buffer, &type, &bytes);
	header->class    = buffer[0];
	header->complex  = buffer[1] & 0x10;
	header->global   = buffer[1] & 0x20;
	header->logical  = buffer[1] & 0x40;
	header->sparce_ceros = * (uint32_t*) (buffer+4);
	buffer += PLUS64( bytes );

	readMATtag( &buffer, &type, &bytes);
	header->dimentions = bytes / 4;
	header->shape = malloc( bytes );
	memcpy( (char*) header->shape, buffer, bytes );
	buffer += PLUS64( bytes );

	readMATtag( &buffer, &type, &bytes);
	header->name = malloc( bytes + 1 );
	header->name[bytes] = 0;
	memcpy( (char*) header->name, buffer, bytes );
	buffer += PLUS64( bytes );

	switch( header->class ){
		case mxCELL_CLASS   :
		case mxSTRUCT_CLASS :
		case mxOBJECT_CLASS :
		case mxCHAR_CLASS   :
		case mxSPARSE_CLASS :
			matrix->content = NULL;
		case mxUINT8_CLASS  :
		case mxINT8_CLASS   :
		case mxUINT16_CLASS :
		case mxINT16_CLASS  :
		case mxUINT32_CLASS :
		case mxINT32_CLASS  :
		case mxUINT64_CLASS :
		case mxINT64_CLASS  :
		case mxSINGLE_CLASS :
		case mxDOUBLE_CLASS :
			matrix->content = loadMATmiNumericMatrix( buffer, size, header );
		break;
		case UNKHOW_ARRAY   :
		default:
			matrix->content = NULL;
	}
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

void * loadMAT( char * buffer, uint32_t * data_type, uint32_t * size){
	readMATtag( & buffer, data_type, size );
	switch( * data_type ){
		case miMATRIX: return loadMATmiMatrix( buffer, * size );
	}
	char * MAT = malloc( * size );
	memcpy( MAT, buffer, * size );
	return MAT;
}

void * readMATnext( int fd, uint32_t * data_type, uint32_t * size ){
	if( ! read( fd, data_type, 4 ) ) return NULL;

	if( * data_type & 0xFFFF0000 ){
		* size      = (* data_type & 0xFFFF0000) >> 16;
		* data_type =  * data_type & 0x0000FFFF;
	} else read( fd, size, 4 );

	void * MAT = NULL;
	if( * data_type == miCOMPRESSED ){
		char * compressed = malloc( * size );
		read( fd, compressed, * size );
		char * uncompressed = zundo( compressed, size );
		free( compressed );
		MAT = loadMAT( uncompressed, data_type, size );
		free( uncompressed );
		return MAT;
	}

	char * buffer = malloc( * size );
	MAT = loadMAT( buffer, data_type, size );
	free( buffer );
	return MAT;
}

void run( char * path ){
	int fd = open( path, O_RDONLY );
	if( fd < 0 ) die("fail on open %s:", path );
	
	printMATheader( readMATheader( fd ) );

	uint32_t data_type, num_bytes;

	void * object;

	while( (object = readMATnext( fd, & data_type, & num_bytes )) ){
		if( data_type == miMATRIX )
			printMATmiMatrixHeader( ((miMatrix*)object)->header );
		else
			printf("Got un-readable %s.\n", mat_data_string[data_type]);
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
