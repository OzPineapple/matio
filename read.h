#ifndef READ_H
#define READ_H

#define ZCHUNK 1024*160

#define FOREACH_MATARRAY( FUN ) \
	FUN( UNKHOW_ARRAY ) \
	FUN( mxCELL_CLASS ) \
	FUN( mxSTRUCT_CLASS ) \
	FUN( mxOBJECT_CLASS ) \
	FUN( mxCHAR_CLASS ) \
	FUN( mxSPARSE_CLASS ) \
	FUN( mxDOUBLE_CLASS ) \
	FUN( mxSINGLE_CLASS ) \
	FUN( mxINT8_CLASS ) \
	FUN( mxUINT8_CLASS ) \
	FUN( mxINT16_CLASS ) \
	FUN( mxUINT16_CLASS ) \
	FUN( mxINT32_CLASS ) \
	FUN( mxUINT32_CLASS ) \
	FUN( mxINT64_CLASS ) \
	FUN( mxUINT64_CLASS )

#define FOREACH_MATTYPE( FUN ) \
	FUN( UNKHOW_TYPE ) \
	FUN( miINT8 ) \
	FUN( miUINT8 ) \
	FUN( miINT16 ) \
	FUN( miUINT16 ) \
	FUN( miINT32 ) \
	FUN( miUINT32 ) \
	FUN( miSINGLE ) \
	FUN( Reserved8 ) \
	FUN( miDOUBLE ) \
	FUN( Reserved10 ) \
	FUN( Reserved11 ) \
	FUN( miINT64 ) \
	FUN( miUINT64 ) \
	FUN( miMATRIX ) \
	FUN( miCOMPRESSED ) \
	FUN( miUTF8 ) \
	FUN( miUTF16 ) \
	FUN( miUTF32 ) 

#define GENERATE_ENUM( ENUM ) ENUM,
#define GENERATE_STRING( ENUM ) #ENUM,

enum mat_data_type { FOREACH_MATTYPE( GENERATE_ENUM ) };
static const char * mat_data_string[] = { FOREACH_MATTYPE( GENERATE_STRING ) };
enum mat_array_type { FOREACH_MATARRAY( GENERATE_ENUM ) };
static const char * mat_array_string[] = { FOREACH_MATARRAY( GENERATE_STRING ) };

typedef struct Matrix_t {
	char * name;
	unsigned int dims_size;
	unsigned int  * dims;
	unsigned char * data;
	enum mat_data_type type;
} Matrix;

typedef struct {
	unsigned int nodes;
	unsigned int edges;
	unsigned int *i;
	unsigned int *j;
	double *w;
} WeightMatrix;

WeightMatrix * readWeightMatrix( int fd );

#endif
