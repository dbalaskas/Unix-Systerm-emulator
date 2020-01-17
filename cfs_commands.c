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

// superBlock sB;

int cfs_workwith(char *filename,bool create_called)
{
	int	fd = -1, ignore = 0, n, sum = 0;

	CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);

	if(create_called == false)
	{
		CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		while(sum < sizeof(superBlock))
		{
			CALL(read(fd,(&sB)+sum,sizeof(superBlock)),-1,"Error reading from cfs file: ",2,n);
			sum += n;
		}
	}

	return fd;
}

bool cfs_touch(int fd,char *filename,touch_mode mode)
{
	int	ignore = 0, move;
	bool	touched;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	if(mode == CRE)
	{
		int		n, sum = 0, i;
        unsigned int blocknum, parent_blocknum;
		char		*split, *temp, *parent_name = (char*)malloc((sB.filenameSize)*sizeof(char));
		time_t		curr_time;
		MDS		metadata, parent_mds;
		Datastream	data, parent_data;

		if(strlen(filename)+1 > sB.filenameSize)
		{
			printf("Input error, too long filename.\n");
			free(parent_name);
			return false;
		}

		split = strtok(filename,"/");									//keep parent's name
		temp = strtok(NULL,"/");
		if(temp == NULL)
			strcpy(parent_name,"/");						//TEMPORARY, CASE OF cd!
		while(temp != NULL)
		{
			strcat(parent_name,split);
			split = temp;
			temp = strtok(NULL,"/");
		}
		parent_blocknum = traverse_cfs(fd,parent_name,1);							//find parent
		if(!parent_blocknum)
		{
			printf("Error, could not find parent directory in cfs.\n");
			free(parent_name);
			return false;
		}
		move = ((sB.blockSize)*(parent_blocknum)) + sB.filenameSize;
		CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		while(sum < sizeof(MDS))
		{
			CALL(read(fd,(&parent_mds)+sum,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
			sum += n;
		}
		parent_data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));
		sum = 0;
		while(sum < (sB.maxDatablockNum*sizeof(unsigned int)))
		{
			CALL(read(fd,parent_data.datablocks,sB.maxDatablockNum*sizeof(unsigned int)),-1,"Error reading from cfs file: ",2,n);
			sum += n;
		}
		for(i=0; i<sB.maxDatablockNum; i++)
			if(parent_data.datablocks[i] == 0)
				break;
		if(i == sB.maxDatablockNum)
		{
			printf("Not enough space in parent directory to create file %s.\n",filename);
			free(parent_name);
			free(parent_data.datablocks);
			return false;
		}

		blocknum = getLocation(fd,0,SEEK_END);							//will move ptr to the end of file
		sum = 0;
		while(sum < (strlen(filename)+1))								//new file's name
		{
			CALL(write(fd,filename+sum,strlen(filename)+1),-1,"Error writing in cfs file: ",3,n);
			sum += n;
		}
		move = sB.filenameSize - (strlen(filename)+1);
		CALL(lseek(fd,move,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
		sB.nodeidCounter++;
		metadata.nodeid = sB.nodeidCounter;
		metadata.size = 0;
		metadata.type = File;
		metadata.parent_nodeid = parent_mds.nodeid;
		curr_time = time(NULL);
		metadata.creation_time = curr_time;
		metadata.access_time = curr_time;
		metadata.modification_time = curr_time;
		sum = 0;
		while(sum < sizeof(MDS))								//new file's metadata
		{
			CALL(write(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error writing in cfs file: ",3,n);
			sum += n;
		}
		data.datablocks = (unsigned int*)malloc(sB.maxDatablockNum*sizeof(unsigned int));
		for(int j=0; j<sB.maxDatablockNum; j++)							//new file's data
		{
			data.datablocks[j] = 0;
			sum = 0;
			while(sum < sizeof(unsigned int))
			{
				CALL(write(fd,&(data.datablocks[j])+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
				sum += n;
			}
		}

		parent_data.datablocks[i] = blocknum;
		move = ((sB.blockSize)*(parent_blocknum)) + sB.filenameSize + sizeof(MDS) + (i*sizeof(unsigned int));
		CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		sum = 0;
		while(sum < sizeof(unsigned int))								//update parent's data
		{
			CALL(write(fd,&(parent_data.datablocks[i])+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
			sum += n;
		}

		free(parent_name);
		free(parent_data.datablocks);
		free(data.datablocks);
		touched = true;
	}
	else
	{
		int		n, sum = 0;
        unsigned int blocknum;
		time_t		curr_time;
		MDS		metadata;

		blocknum = traverse_cfs(fd,filename,1);
		if(!blocknum)
		{
			printf("Couldn't find file %s in cfs.\n",filename);
			touched = false;
		}
		else
		{
			move = ((blocknum)*(sB.blockSize)) + sB.filenameSize;
			CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			while(sum < sizeof(MDS))
			{
				CALL(read(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
				sum += n;
			}
			curr_time = time(NULL);
			if(mode == ACC)
				metadata.access_time = curr_time;
			else if(mode == MOD)
				metadata.modification_time = curr_time;

			sum = 0;
			while(sum < sizeof(MDS))
			{
				CALL(write(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error writing in cfs file: ",3,n);
				sum += n;
			}
			touched = true;
		}
	}

	return touched;
}

int cfs_create(char *filename,int bSize,int nameSize,int maxFSize,int maxDirFileNum)
{
	int		fd = 0, ignore = 0, n, sum = 0;
	time_t		current_time;
	MDS		metadata;
	Datastream	data;

	CALL(creat(filename, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,fd);

	sB.blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	sB.filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	sB.maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	sB.maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	sB.metadataSize = (sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) / sB.blockSize;
	sB.root = 1;
	sB.blockCounter = 1;										//assume block-counting is zero-based
	sB.nodeidCounter = 1;
	sB.nextSuperBlock = -1;
	sB.stackSize = 0;

	while(sum < sizeof(superBlock))
	{
		CALL(write(fd,(&sB)+sum,sizeof(superBlock)),-1,"Error writing in cfs file: ",3,n);
		sum += n;
	}

	CALL(lseek(fd,sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);		//go to second block

	CALL(write(fd,"/",2),-1,"Error writing in cfs file: ",3,ignore);				//write root's name
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);	//leave filenameSize space free

	metadata.nodeid = 1;
	metadata.size = 0;
	metadata.type = Directory;
	metadata.parent_nodeid = 1;

	current_time = time(NULL);
	metadata.creation_time = current_time;
	metadata.access_time = current_time;
	metadata.modification_time = current_time;

	sum = 0;
	while(sum < sizeof(MDS))									//root's metadata
	{
		CALL(write(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error writing in cfs file: ",3,n);
		sum += n;
	}
	data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));			//root's data
	for(int i=0; i<(sB.maxDatablockNum); i++)
	{
		data.datablocks[i] = 0;
		sum = 0;
		while(sum < sizeof(unsigned int))
		{
			CALL(write(fd,&(data.datablocks[i])+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
			sum += n;
		}
	}

//	int size = sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
//	InodeTable = (char*)malloc(size);

	CALL(close(fd),-1,"Error closing file for cfs: ",4,ignore);

	free(data.datablocks);
	return fd;
}

