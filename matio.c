#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>

#include "util.h"
#include "matio.h"

void readHeader( int fd ){
	lseek( fd, 128, SEEK_SET );
	return;
}

void writeHeader( int fd ){
	size_t padding = 0;
	padding += write( fd, "MATLAB 5.0 MAT-file, Platform: ", 31 ); 
	struct utsname osinfo;
	uname( & osinfo );
	if( strlen(osinfo.nodename) )
		padding += write( fd, osinfo.nodename, strlen(osinfo.nodename) );
	else
		padding += write( fd, osinfo.sysname, strlen(osinfo.sysname) );
	padding += write( fd, " ", 1 );
	padding += write( fd, osinfo.machine, strlen(osinfo.machine) );
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char date[128];
	strftime(date, sizeof(date), "%c", tm );
	padding += write( fd, ", Created on: ", 14 ); 
	padding += write( fd, date, strlen( date ) );
	if( padding > 116 )
		die("Writing header too long");
	padding = 116 - padding;
	for( char i=0, cero = 0; i<padding; i++)
		write(fd, & cero, 1 );
	long output=0;
	write( fd, &output, 8 ); // subsystem offset info
	output = 0x0100;
	write( fd, &output, 2 ); // version
	write( fd, "IM", 2 ); // endian indicator
}

char * loadNextMat( int fd, uint32_t * size ){
	char * buff = malloc( 8 );
	if( ! read( fd, buff, 8 ) ) return NULL;
	buff = erealloc( buff, 8 + ( * (int*) (buff+4) ) );
	read( fd, buff + 8, ( * (int*) (buff+4) ) );
	* size = *(int*)(buff+4)+8;
	return buff;
}
