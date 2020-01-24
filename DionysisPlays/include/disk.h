/* FILE: disk.h */
#include "list.h"

#define BLOCK_SIZE 512
#define MAX_FILE_SIZE 51200
#define MAX_DIRECTORY_FILE_NUMBER 10
#define FILENAME_SIZE 100
#define MAX_FILENAME_SIZE 252

typedef struct superblock {
	int nextSuperBlock;
	int blockSize;
	int filenameSize;
	unsigned int maxFileSize;
	int maxFilesPerDir;
	int maxFileDatablockNum;
	int maxDirDatablockNum;
	int maxDatablockNum;
//   	int metadataBlocksNum;
	int maxEntitiesPerBlock;		// Entity: filename + nodeid (for directrories' data)
	int root;
	int blockCounter;
	int nodeidCounter;
	int ListSize;
    	int iTableBlocksNum;
    	int iTableCounter;
} superBlock;


extern char *inodeTable;
extern int inodeSize;
extern superBlock sB;
extern List *holes;
extern int cfs_current_nodeid;
