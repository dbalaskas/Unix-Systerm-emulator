/* FILE: cfs_commands.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_commands.h"

/*
1. cfs workwith <FILE>
2. cfs mkdir <DIRECTORIES>
3. cfs touch <OPTIONS> <FILES>
4. cfs pwd
5. cfs cd <PATH>
6. cfs ls <OPTIONS> <FILES>
7. cfs cp <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>
8. cfs cat <SOURCE FILES> -o <OUTPUT FILE>
9. cfs ln <SOURCE FILES> <OUTPUT FILE>
10. cfs mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>
11. cfs rm <OPTIONS> <DESTINATIONS>
12. cfs import <SOURCES> ... <DIRECTORY>
13. cfs export <SOURCES> ... <DIRECTORY>
14. cfs create <OPTIONS> <FILE>. Δημιουργία ενός cfs στο αρχείο <FILE>.
*/

void cfs_workwith(char *filename)
{
	int		fd = 0, ignore = 0;
	time_t		current_time;
	superBlock	sB;
	MDS		metadata;

	CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);

	CALL(read(fd,&sB,sizeof(sB)),-1,"Error reading from cfs file: ",3,ignore);		//read superBlock (first block)
	CALL(lseek(fd,sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);	//go to second block

	CALL(write(fd,"/",2),-1,"Error writing in cfs file: ",3,ignore);			//write root's name
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);//leave filenameSize space free

	metadata.nodeid = 0;
	metadata.size = sB.maxFileSize;
	metadata.type = Directory;
	metadata.parent_nodeid = 0;			//??

	current_time = time(NULL);
	metadata.creation_time = current_time;
	metadata.access_time = current_time;
	metadata.modification_time = current_time;
	CALL(write(fd,&metadata,sizeof(metadata)),-1,"Error writing in cfs file: ",3,ignore);
	for(int i=0; i<(sB.maxDatablockNum); i++)
	{
		CALL(write(fd,"-1",2),-1,"Error writing in cfs file: ",3,ignore);
//		CALL(write(fd,"0",2),-1,"Error writing in cfs file: ",3,ignore);		//2-dimensional array for datablocknum+offset
	}
}

int cfs_create(char *filename,int bSize,int nameSize,int maxFSize,int maxDirFileNum)
{
	int		fd = 0, ignore = 0;
	superBlock	sB;

	CALL(creat(filename, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,fd);

	CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,ignore);

	sB.cfsFileDesc = fd;
	sB.blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	sB.filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	sB.maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	sB.maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	sB.maxDatablockNum = (sB.maxFileSize) / (sB.blockSize);
	sB.metadataSize = -1;
	sB.root = -1;
	sB.blockCounter = -1;				//assume block-counting is zero-based
	sB.nextSuperBlock = -1;
	sB.stackSize = 0;

	CALL(write(fd,&sB,sizeof(sB)),-1,"Error writing in cfs file: ",3,ignore);
	CALL(close(fd),-1,"Error closing file for cfs: ",4,ignore);

	return fd;
}

//	CALL(write(fd,filename,strlen(filename)+1),-1,"Error writing in cfs file: ",3);
//	CALL(lseek(fd,strlen(filename)+1,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5);

