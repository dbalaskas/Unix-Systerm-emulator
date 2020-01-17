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

superBlock sB;

int cfs_workwith(char *filename,bool open_cfs)
{
	int	fd = -1, ignore = 0, n, sum = 0;
	if(open_cfs == true)
	{
        perror("Error: a cfs file is already open. Close it if you want to open other cfs file.\n");
    } else {
	    CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);
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
		char		*split, *temp, *parent_name = (char*)malloc((sB.filenameSize)*sizeof(char));
		time_t		curr_time;
		Location	loc, parent_loc;
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
		parent_loc = traverse_cfs(fd,parent_name,1,0);							//find parent
		if(!parent_loc.blocknum)
		{
			printf("Error, could not find parent directory in cfs.\n");
			free(parent_name);
			return false;
		}
		move = ((sB.blockSize)*(parent_loc.blocknum)) + parent_loc.offset + sB.filenameSize;
		CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		while(sum < sizeof(MDS))
		{
			CALL(read(fd,(&parent_mds)+sum,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
			sum += n;
		}
		parent_data.datablocks = (Location*)malloc((sB.maxDatablockNum)*sizeof(Location));
		sum = 0;
		while(sum < (sB.maxDatablockNum*sizeof(Location)))
		{
			CALL(read(fd,parent_data.datablocks,sB.maxDatablockNum*sizeof(Location)),-1,"Error reading from cfs file: ",2,n);
			sum += n;
		}
		for(i=0; i<sB.maxDatablockNum; i++)
			if((parent_data.datablocks[i]).blocknum == 0)
				break;
		if(i == sB.maxDatablockNum)
		{
			printf("Not enough space in parent directory to create file %s.\n",filename);
			free(parent_name);
			free(parent_data.datablocks);
			return false;
		}

		loc = getLocation(fd,0,SEEK_END);							//will move ptr to the end of file
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
		data.datablocks = (Location*)malloc(sB.maxDatablockNum*sizeof(Location));
		for(int j=0; j<sB.maxDatablockNum; j++)							//new file's data
		{
			(data.datablocks[j]).blocknum = 0;
			(data.datablocks[j]).offset = 0;
			sum = 0;
			while(sum < sizeof(Location))
			{
				CALL(write(fd,&(data.datablocks[j])+sum,sizeof(Location)),-1,"Error writing in cfs file: ",3,n);
				sum += n;
			}
		}

		(parent_data.datablocks[i]).blocknum = loc.blocknum;
		(parent_data.datablocks[i]).offset = loc.offset;
		move = ((sB.blockSize)*(parent_loc.blocknum)) + parent_loc.offset + sB.filenameSize + sizeof(MDS) + (i*sizeof(Location));
		CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		sum = 0;
		while(sum < sizeof(Location))								//update parent's data
		{
			CALL(write(fd,&(parent_data.datablocks[i])+sum,sizeof(Location)),-1,"Error writing in cfs file: ",3,n);
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
		time_t		curr_time;
		Location	loc;
		MDS		metadata;

		loc = traverse_cfs(fd,filename,1,0);
		if(!loc.blocknum)
		{
			printf("Couldn't find file %s in cfs.\n",filename);
			touched = false;
		}
		else
		{
			move = ((loc.blocknum)*(sB.blockSize)) + loc.offset + sB.filenameSize;
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
	sB.maxDatablockNum = (sB.maxFileSize) / (sB.blockSize);
	sB.metadataSize = sB.filenameSize + sizeof(MDS);
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
	data.datablocks = (Location*)malloc((sB.maxDatablockNum)*sizeof(Location));			//root's data
	for(int i=0; i<(sB.maxDatablockNum); i++)
	{
		(data.datablocks[i]).blocknum = 0;
		(data.datablocks[i]).offset = 0;
		sum = 0;
		while(sum < sizeof(Location))
		{
			CALL(write(fd,&(data.datablocks[i])+sum,sizeof(Location)),-1,"Error writing in cfs file: ",3,n);
			sum += n;
		}
	}

	CALL(close(fd),-1,"Error closing file for cfs: ",4,ignore);

	free(data.datablocks);
	return fd;
}

bool cfs_close(int fileDesc,bool open_cfs) {
    int ignore = 0;
    if(fileDesc != -1 && open_cfs == true)
    {
        update_superBlock(fileDesc);
        CALL(close(fileDesc),-1,"Error closing file for cfs: ",4,ignore);
        open_cfs = false;
        return true;
    }
    return false;
}