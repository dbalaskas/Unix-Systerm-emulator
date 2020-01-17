/* FILE: disk.h */

#define BLOCK_SIZE 512
#define MAX_FILE_SIZE 51200
#define MAX_DIRECTORY_FILE_NUMBER 10
#define FILENAME_SIZE 100

typedef struct superblock {
	int blockSize;
	int filenameSize;
	int maxFileSize;
	int maxFilesPerDir;
	int maxDatablockNum;
	int metadataSize;
	int root;
	int blockCounter;
	int nodeidCounter;
	int nextSuperBlock;
	int stackSize;
} superBlock;

extern superBlock sB;

void writeStack();
void getStack();
