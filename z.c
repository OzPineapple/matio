#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "util.h"
#include "z.h"

char * zundo( char * compress , unsigned int size ){
	int ret;
	z_stream strm;

	strm.zalloc    = Z_NULL;
	strm.zfree     = Z_NULL;
	strm.opaque    = Z_NULL;
	strm.avail_in  = size;
	strm.next_in   = (unsigned char *) compress;

	ret = inflateInit(&strm);
	if( ret != Z_OK ) die("Can't uncompress input:");


	unsigned int blocks = 1;
	unsigned char * uncompress = NULL;

	do{
		uncompress = erealloc( uncompress, blocks * ZCHUNK );

		strm.avail_out = ZCHUNK;
		strm.next_out  = uncompress + strm.total_out;

		ret = inflate( &strm, Z_NO_FLUSH );

		switch( ret ){
			case Z_STREAM_ERROR:
				die("uncompress data failed by Strem Error:");
			case Z_DATA_ERROR:
				die("uncompress data failed by Data Error:");
			case Z_MEM_ERROR:
				die("uncompress data failed by Mem Error:");
				inflateEnd(&strm);
		}
		blocks++;
	} while( strm.avail_out == 0 );
	
	if( blocks * ZCHUNK > strm.total_out )
		uncompress = realloc( uncompress, strm.total_out );
	
	inflateEnd(&strm);

	return (char*) uncompress;
}

char * zdo( char * uncompress, unsigned int size, unsigned int * out_size){
	int ret;
	z_stream strm;

	strm.zalloc    = Z_NULL;
	strm.zfree     = Z_NULL;
	strm.opaque    = Z_NULL;
	strm.avail_in  = size;
	strm.next_in   = (unsigned char *) uncompress;

	ret = deflateInit(&strm, 9);
	if( ret != Z_OK ) die("Can't compress input:");

	unsigned int blocks = 1;
	unsigned char * compress = NULL;

	do{
		compress = erealloc( compress, blocks * ZCHUNK );

		strm.avail_out = ZCHUNK;
		strm.next_out  = compress + strm.total_out;

		ret = deflate( &strm, Z_FINISH );

		switch( ret ){
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				deflateEnd(&strm);
				die("compress data failed:");
		}
		blocks++;
	} while( strm.avail_out == 0 );

	if( blocks * ZCHUNK > strm.total_out )
		compress = realloc( compress, strm.total_out );
	* out_size = strm.total_out;
	
	deflateEnd(&strm);

	return (char*) compress;
}
