#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "matio.h"
#include "z.h"

void compress( int argc, char ** argv ){
	for( int i=1; i<argc; i++ ){
		char * path = malloc( strlen( argv[i] ) + 3 );
		strcpy( path, argv[i] );
		strcpy( path + strlen( argv[i] ) - 3, "z.mat" );

		int fd_in  = open( argv[i], O_RDONLY );
		int fd_out = open( path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

		readHeader( fd_in );
		writeHeader( fd_out );
		uint32_t usize,csize,type;
		char * ubuff, * cbuff;
		while( (ubuff = loadNextMat( fd_in, & usize )) != NULL ){
			if( *(int*) ubuff == 15 ) write( fd_out, ubuff, usize );
			else {
				cbuff = zdo(ubuff, usize, & csize);
				type = 15;
				write( fd_out, & type, 4 );
				write( fd_out, & csize, 4 );
				write( fd_out, cbuff, csize );
			}
		}
		close( fd_in );
		close( fd_out );
	}
}

void uncompress( int argc, char ** argv ){
	for( int i=1; i<argc; i++ ){
		char * path = malloc( strlen( argv[i] ) + 4 );
		strcpy( path, argv[i] );
		strcpy( path + strlen( argv[i] ) - 4, "nz.mat" );

		int fd_in  = open( argv[i], O_RDONLY );
		int fd_out = open( path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

		readHeader( fd_in );
		writeHeader( fd_out );
		uint32_t csize;
		char * ubuff, * cbuff;
		while( (cbuff = loadNextMat( fd_in, & csize )) != NULL ){
			if( * ( (int*) cbuff ) != 15 ) write( fd_out, cbuff, csize );
			else {
				ubuff = zundo(cbuff, csize);
				write( fd_out, ubuff, 4 );
				write( fd_out, ubuff+4, 4 );
				write( fd_out, ubuff+8, *(int*)(ubuff+4) );
			}
		}
		close( fd_in );
		close( fd_out );
	}
}

int main( int argc, char ** argv ){
	for( int i=1; i<argc; i++ )
		if( ! strcmp( argv[i], "compress" ) )
			compress( argc - i, argv + i );
		else if( ! strcmp( argv[i], "uncompress" ) )
			uncompress( argc - i, argv + i );
}
