#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>                              //creat, open
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

void cfs_create(char *filename,int *bSize,int *filenameSize,int *maxFSize,int *maxDirFileNum)
{
        int             fd;
        superBlock      sB;

        fd = creat(filename, S_IRWXU | S_IXGRP);
        if(fd == -1)
        {
                 perror("Error creating file for cfs: ");
                exit(1);
        }

        CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2);

        sB.cfsFileDesc = fd;
        sB.blockSize = (bSize != NULL) ? (*bSize) : BLOCK_SIZE;
        sB.maxFileSize = (maxFSize != NULL) ? (*maxFSize) : MAX_FILE_SIZE;
        sB.maxFilesPerDir = (maxDirFileNum != NULL) ? (*maxDirFileNum) : MAX_DIRECTORY_FILE_NUMBER;
        sB.maxDatablockNum = (sB.maxFileSize) / (sB.blockSize);
        sB.metadataSize = -1;
        sB.root = -1;
        sB.blockCounter = -1;                           //assume block-counting is zero-based
        sB.nextSuperBlock = -1;
        sB.stackSize = 0;

        CALL(write(fd,&sB,sizeof(sB)),-1,"Error writing in cfs file: ",3);
        CALL(close(fd),-1,"Error closing file for cfs: ",4);
}
