#ifndef MAT_H
#define MAT_H

#define ZCHUNK 1024*160

#define FOREACH_MATARRAY( FUN ) \
	FUN( UNKHOW_ARRAY   ) \
	FUN( mxCELL_CLASS   ) \
	FUN( mxSTRUCT_CLASS ) \
	FUN( mxOBJECT_CLASS ) \
	FUN( mxCHAR_CLASS   ) \
	FUN( mxSPARSE_CLASS ) \
	FUN( mxDOUBLE_CLASS ) \
	FUN( mxSINGLE_CLASS ) \
	FUN( mxINT8_CLASS   ) \
	FUN( mxUINT8_CLASS  ) \
	FUN( mxINT16_CLASS  ) \
	FUN( mxUINT16_CLASS ) \
	FUN( mxINT32_CLASS  ) \
	FUN( mxUINT32_CLASS ) \
	FUN( mxINT64_CLASS  ) \
	FUN( mxUINT64_CLASS )

#define FOREACH_MATTYPE( FUN ) \
	FUN( UNKHOW_TYPE  ) \
	FUN( miINT8       ) \
	FUN( miUINT8      ) \
	FUN( miINT16      ) \
	FUN( miUINT16     ) \
	FUN( miINT32      ) \
	FUN( miUINT32     ) \
	FUN( miSINGLE     ) \
	FUN( Reserved8    ) \
	FUN( miDOUBLE     ) \
	FUN( Reserved10   ) \
	FUN( Reserved11   ) \
	FUN( miINT64      ) \
	FUN( miUINT64     ) \
	FUN( miMATRIX     ) \
	FUN( miCOMPRESSED ) \
	FUN( miUTF8       ) \
	FUN( miUTF16      ) \
	FUN( miUTF32      ) 

#define GENERATE_ENUM( ENUM )    ENUM,
#define GENERATE_STRING( ENUM ) #ENUM,

enum mat_data_type  { FOREACH_MATTYPE(  GENERATE_ENUM ) };
enum mat_array_type { FOREACH_MATARRAY( GENERATE_ENUM ) };
static const char * mat_data_string[]  = { FOREACH_MATTYPE(  GENERATE_STRING ) };
static const char * mat_array_string[] = { FOREACH_MATARRAY( GENERATE_STRING ) };
static const char mat_data_size[]  = { 0, 1, 1, 2, 2, 4, 4, 4, 0, 8, 0, 0, 8, 8, 0, 0, 0, 0, 0 };
static const char mat_array_size[] = { 0, 0, 0, 0, 0, 0, 8, 4, 1, 1, 2, 2, 4, 4, 8, 8 };

typedef struct {
	uint8_t  text[116];
	uint64_t subsystem_data_offset;
	uint16_t version;
	uint8_t  endian[2];
} MATheader;

typedef struct {
	uint8_t    complex:1;
	uint8_t    global:1;
	uint8_t    logical:1;
	uint8_t  * name;
	uint16_t   class;
	uint32_t   dimentions;
	uint32_t * shape;
	uint32_t   sparce_ceros;
} miMatrixHeader;

typedef struct {
	void * real;
	void * imaginary;
} miNumericMatrix;

typedef struct {
	uint32_t   maxceros;
	int32_t  * row;
	int32_t  * colum;
	void * real;
	void * imaginary;
} miSparseMatrix;

typedef struct {
	uint32_t   length;
	uint8_t  * data;
} miStructMatrix;

typedef struct {
	miMatrixHeader * header;
	void * content;
} miMatrix;

#endif // MAT_H
