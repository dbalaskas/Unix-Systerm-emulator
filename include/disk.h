/* FILE: disk.h */
#include "list.h"

#define BLOCK_SIZE 512
#define MAX_FILE_SIZE 51200
#define MAX_DIRECTORY_FILE_NUMBER 10
#define FILENAME_SIZE 100

typedef struct superblock {
	int nextSuperBlock;
	int blockSize;
	int filenameSize;
	int maxFileSize;
	int maxFilesPerDir;
	int maxDatablockNum;
	// int metadataSize;
   	int metadataBlocksNum;
	int root;
	int blockCounter;
	int nodeidCounter;
	int ListSize;
    	int iTableBlocksNum;
    	int iTableCounter;
} superBlock;

// typedef struct {
//     char *table;
//     int inodeCounter;
// } inodeTable;



extern char *inodeTable;
extern superBlock sB;
extern List *holes;
