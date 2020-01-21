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

int cfs_workwith(char *filename)
{
	int             fd = -1, ignore = 0, sum, n, plus;
	int		offset = 0, listOffset;
	int		overflow_block, read_iblocks, metadata_blocknum;
	unsigned int	current_iblock[sB.metadataBlocksNum];

	CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);
	// We assume that we are in root at first, update global variable of our location
	cfs_current_nodeid = 0;

	// Go to the first block, read superBlock
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_READ(fd,&sB,0,sizeof(superBlock),sizeof(superBlock),sum,n,plus);

	// (first) Block with the inodeTable and the holeList
	overflow_block = sB.nextSuperBlock;
	read_iblocks = sizeof(int);

	// Size of an inodeTable element
	inodeSize = sizeof(bool) + sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
	// Buffer to store info of an inodeTable element
	char buffer[inodeSize-sizeof(bool)];
        int remainingSize, size_to_read;
        int *nodeId;
        inodeTable = (char*)malloc(sB.iTableCounter*inodeSize);
        holes = NULL;
        int numOfiNode = 0;

	// For every element of the inodeTable
	for(int j=0; j<sB.nodeidCounter; j++)
	{
		// Start from the first block with the element's metadata
		metadata_blocknum = 0;
		// Number of bytes to be read from cfs file
		remainingSize = inodeSize-sizeof(bool);
		// For every block with the element's metadata
		for(int i=0; i<sB.metadataBlocksNum; i++)
        	{
			// Go to (the appropriate) block with the inodeTable
			CALL(lseek(fd,(overflow_block*sB.blockSize)+read_iblocks,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			// current_iblock[metadata] = appropriate block with metadata
			SAFE_READ(fd,current_iblock,metadata_blocknum*sizeof(unsigned int),sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);
			// Go to (the appropriate) block with metadata
       	        	CALL(lseek(fd,current_iblock[metadata_blocknum]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			// Will read as many metadata as possible (not outside the current block)
			size_to_read = (remainingSize > sB.blockSize) ? sB.blockSize : remainingSize;
			SAFE_READ(fd,buffer,metadata_blocknum*sB.blockSize,sizeof(char),size_to_read,sum,n,plus);
        		remainingSize -= size_to_read;

			// Go to the next element in inodeTable
			read_iblocks += sizeof(unsigned int);
			// If inodeTable has more elements, it is continued to another block (overflow)
			if(read_iblocks == sB.blockSize)
			{
				// Get overflow block
				CALL(lseek(fd,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(fd,&overflow_block,0,sizeof(int),sizeof(int),sum,n,plus);
				read_iblocks = sizeof(int);
			}
			// Go to the next block of metadata
			metadata_blocknum++;
		}

	            	nodeId = (int*) (buffer + sB.filenameSize);				//update inodeTable
        	    	// Find the right place for the inode ~based on nodeid~ (holes in the inodeTable have to be maintained)
            		while(numOfiNode < *nodeId)
	            	{
        	        	*(inodeTable + offset) = false;                                 //empty
                		numOfiNode++;
                		offset += inodeSize;
	            	}
        	    	*(inodeTable + offset) = true;
			// Copy inodeTable from cfs file to table structure
			memcpy(inodeTable+offset+sizeof(bool), buffer, inodeSize-sizeof(bool));
	            	numOfiNode++;
			// Go to inodeTable's next element
			offset += inodeSize;
		}

		int	difference = sB.iTableBlocksNum*sizeof(unsigned int);
		int	overflow_prev, overflow_next;
		int	remainingblockSize = sB.blockSize - sizeof(int);
		// Start from first block with the inodeTable
		overflow_block = sB.nextSuperBlock;
		// Find the end of the inodeTable
		while(difference > 0)
		{
			CALL(lseek(fd,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			overflow_prev = overflow_block;
			SAFE_READ(fd,&overflow_block,0,sizeof(int),sizeof(int),sum,n,plus);

			// If there is another overflow block
			if((difference > remainingblockSize) || (difference == remainingblockSize && sB.ListSize > 0))
				difference -= remainingblockSize;
			// InodeTable ends in current block and there is space left in the same block
			else
			{
				overflow_next = overflow_block;
				overflow_block = overflow_prev;					//block where the inodeTable ends
				break;
			}
		}

		unsigned int	blockNum;
		// Number of bytes left in current block, after the inodeTable
		int	freeSpaces = remainingblockSize - difference;
		// Read list of holes
		for(int i=0; i<sB.ListSize; i++)
        	{
			// If there is enough space in current block to read
			if(freeSpaces > 0)
			{
				listOffset = overflow_block*sB.blockSize + (sB.blockSize - freeSpaces);
        			CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			}
			else
			{
				// If current block already has an overflow
				if(overflow_next != -1)
				{
					overflow_block = overflow_next;
					listOffset = overflow_block*sB.blockSize;
	            			CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_WRITE(fd,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
				}
				// If an empty block is needed
				else
				{
					overflow_block = getEmptyBlock();
					listOffset = overflow_block*sB.blockSize;
		            		CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

		            		overflow_next = -1;
					SAFE_WRITE(fd,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
				}

				freeSpaces = remainingblockSize;
			}
			// Read a list node (fileptr is on the right spot)
			SAFE_READ(fd,&blockNum,0,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);
			// Push it in List structure
            		addNode(holes, blockNum);
			// Space in current block is reduced by one list node
			freeSpaces -= sizeof(unsigned int);
        	}

	return fd;
}

bool cfs_touch(int fd,char *filename,touch_mode mode)
{
	int	ignore = 0;
	bool	touched, busy;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	if(mode == CRE)
	{
		unsigned int	start, new_nodeid;
		int		parent_nodeid, i, offset;
		char		*split, *temp;
	 	char		parent_name[sB.filenameSize], new_name[sB.filenameSize];
		time_t		curr_time;
		MDS		*current_mds, *metadata;
		Datastream	parent_data, data;

		// If path starts with "/"
		if(!strncmp(filename,"/",1))
		{
			// Parent path will also start with "/"
			strcpy(parent_name,"/");
			// Traversing cfs will start from the root (nodeid = 0)
			start = 0;
			// Get next entity in path
			split = strtok(filename,"/");
		}
		else
		{
			// Get first entity in path
			split = strtok(filename,"/");
			// If path starts from current directory's parent
			if(!strcmp(split,".."))
			{
				current_mds = (MDS*)(inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				start = current_mds->parent_nodeid;
				strcpy(parent_name,"../");
				split = strtok(NULL,"/");
			}
			else	//if(!strcmp(split,".") || strcmp(split,"/"))
			{
				// If path starts from current directory
				start = cfs_current_nodeid;
				strcpy(parent_name,"./");
				if(!strcmp(split,"."))
					// Get next entity in path
					split = strtok(NULL,"/");
			}
		}

		// If there is another entity in path
		if(split != NULL)
		{
			temp = strtok(NULL,"/");
			if(temp == NULL)
				strcpy(new_name,split);
		}
		else
		{
			printf("Input error, %s is not a file.\n",filename);
			return false;
		}
		// Get parent's name and new file's name
		while(temp != NULL)
		{
			strcat(parent_name,split);
			split = temp;
			temp = strtok(NULL,"/");
			if(temp != NULL)
				strcat(parent_name,"/");
			else
				strcpy(new_name,split);
		}
		// If new file's name is too long
		if(strlen(new_name)+1 > sB.filenameSize)
		{
			printf("Input error, too long filename.\n");
			return false;
		}
		// Find parent directoy in cfs (meaning in inodeTable)
		parent_nodeid = traverse_cfs(parent_name,start);
		if(parent_nodeid == -1)
		{
			printf("Error, could not find parent directory in cfs.\n");
			return false;
		}
		parent_data.datablocks = (unsigned int*)(inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

		// Update parent directory's data
		for(i=0; i<sB.maxDatablockNum; i++)
			if(parent_data.datablocks[i] == 0)
				break;
		if(i == sB.maxDatablockNum)
		{
			printf("Not enough space in parent directory to create file %s.\n",filename);
			return false;
		}

		// Find an empty space in inodeTable
		new_nodeid = getTableSpace();
		parent_data.datablocks[i] = new_nodeid;

		sB.nodeidCounter++;
		sB.iTableCounter++;
		// Allocate space for another element in inodeTable
		inodeTable = (char*)realloc(inodeTable,sB.iTableCounter*inodeSize);
		offset = new_nodeid*inodeSize;
		// Fill empty space with new file
		busy = true;
		*(bool*) (inodeTable + offset) = busy;

		offset += sizeof(bool);
		strcpy(inodeTable + offset,new_name);

		offset += sB.filenameSize;
		metadata = (MDS*) (inodeTable + offset);
		metadata->nodeid = new_nodeid;
		metadata->size = 0;
		metadata->type = File;
		metadata->parent_nodeid = parent_nodeid;
		curr_time = time(NULL);
		metadata->creation_time = curr_time;
		metadata->access_time = curr_time;
		metadata->modification_time = curr_time;

		offset += sizeof(MDS);
		data.datablocks = (unsigned int*) (inodeTable + offset);
		for(i=0; i<sB.maxDatablockNum; i++)
			data.datablocks[i] = 0;

		touched = true;
	}
	else
	{
		unsigned int	start;
	        int		nodeid;
		char		temp[sB.filenameSize];
		time_t		curr_time;
		MDS		*current_mds, *metadata;

		// If path starts with "/"
		if(!strncmp(filename,"/",1))
			// Traversing cfs will start from the root (nodeid = 0)
			start = 0;
		else
		{
			char	*split;
			char	strtk_filename[sB.filenameSize];
			// Will be used in strtok (changes initial string)
			strcpy(strtk_filename,filename);

			// Get first entity in path
			split = strtok(strtk_filename,"/");
			// If path starts from current directory's parent
			if(!strcmp(split,".."))
			{
				current_mds = (MDS*)(inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				start = current_mds->parent_nodeid;
			}
			else//if(!strcmp(split,".") || strcmp(split,"/"))
			{
				// If path starts from current directory
				start = cfs_current_nodeid;
				if(strcmp(split,".") && strcmp(split,"/"))
				{
					strcpy(temp,"./");
					strcat(temp,filename);
					strcpy(filename,temp);
				}
			}
		}

		nodeid = traverse_cfs(filename,start);
		if(nodeid == -1)
		{
			printf("Couldn't find file %s in cfs.\n",filename);
			touched = false;
		}
		else
		{
			metadata = (MDS*) (inodeTable + nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			curr_time = time(NULL);
			if(mode == ACC)
				metadata->access_time = curr_time;
			else if(mode == MOD)
				metadata->modification_time = curr_time;
		}
			touched = true;
	}

	return touched;
}

void cfs_pwd() {
	// Takes the current nodeid from global variable cfs_current_nodeid
	MDS 	*curr_dir_MDS = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	int 	curr_dir_id = cfs_current_nodeid;

	// Traverse the nodes until the root. For each one save its id in a list path_nodeIds
	List *path_nodeIds = NULL;
	while (curr_dir_id != 0) {
		addNode(path_nodeIds, curr_dir_id);
		curr_dir_id = curr_dir_MDS->parent_nodeid;
		curr_dir_MDS = (MDS *) (inodeTable + curr_dir_id*inodeSize + sizeof(bool));
	}
	// addNode(path_nodeIds, curr_dir_id);

	// For debuging only
	printList(path_nodeIds);

	// Now the list path_nodeIds has the nodeids of each dir on the path to current dir, from start to end.
	// So we pop each one and print its name
	char 	*fileName = NULL;
	if (path_nodeIds == NULL)
		// We are at the root
		printf("/");
	else {
		// We are not at the root
		while (path_nodeIds != NULL) {
			curr_dir_id = pop(path_nodeIds);
			curr_dir_MDS = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			fileName = (inodeTable + curr_dir_id*inodeSize + sizeof(bool));;
			printf("/%s", fileName);
		}
	}
}

void cfs_cd(char *path) {

}

int cfs_create(char *filename,int bSize,int nameSize,int maxFSize,int maxDirFileNum)
{
	int		fd = 0, ignore = 0, sum, n, plus;
	time_t		current_time;
	MDS		metadata;
	Datastream	data;

	CALL(creat(filename, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,fd);
//find if file exists, if so: syscall remove

	// Create superblock
	sB.blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	// Block size has a bottom (512 bytes)
	if(bSize != -1 && bSize < BLOCK_SIZE)
		return -1;
	sB.filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	sB.maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	sB.maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	sB.maxDatablockNum = (sB.maxFileSize) / (sB.blockSize);
	sB.metadataBlocksNum = (sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) / sB.blockSize;
	if(((sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) % sB.blockSize) > 0)
        	sB.metadataBlocksNum++;

	// Assume block-counting is zero based	
	sB.blockCounter = 0;
	// Only root in cfs for now
	sB.nodeidCounter = 1;
	sB.ListSize = 0;
	sB.iTableBlocksNum = sB.nodeidCounter*sB.metadataBlocksNum;
	sB.iTableCounter = sB.nodeidCounter;
	// Get superblock's overflow block (contains the inodeTable and the holeList)
	sB.nextSuperBlock = (int)getEmptyBlock();
	CALL(lseek(fd,sB.nextSuperBlock*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	int	overflow_block = 0;
	SAFE_WRITE(fd,&overflow_block,0,sizeof(int),sizeof(int),sum,n,plus);

///////////////CREATE ENTITY ///////////////////////
	unsigned int	blocknums[sB.metadataBlocksNum];
	// For each block with root's meatdata
	for(int i=0; i<sB.metadataBlocksNum; i++)
	{
		// Get a new block
		blocknums[i] = getEmptyBlock();
		if(i == 0)
			sB.root = blocknums[0];
		SAFE_WRITE(fd,&blocknums[i],0,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);
	}
	
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&sB,0,sizeof(superBlock),sizeof(superBlock),sum,n,plus);

	int	current = 0;
	int	size_to_write, remainingSize, remainingblockSize, writtenSize = 0;
	// Go to the first block with root's metadata
	CALL(lseek(fd,blocknums[current]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	// Write root's name (cannot be larger than a block)
	CALL(write(fd,"/",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);

	// Number of bytes to be written in metadata blocks
	remainingSize = sizeof(MDS);
	remainingblockSize = sB.blockSize - sB.filenameSize;
	// Will write in block as many as possible
	size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;

	metadata.nodeid = 0;
	metadata.size = 0;
	metadata.type = Directory;
	metadata.parent_nodeid = 0;
	current_time = time(NULL);
	metadata.creation_time = current_time;
	metadata.access_time = current_time;
	metadata.modification_time = current_time;

	while(remainingSize > 0)									//write root's metadata
	{
		SAFE_WRITE(fd,&metadata,writtenSize,sizeof(MDS),size_to_write,sum,n,plus);

		remainingSize -= size_to_write;
		remainingblockSize -= size_to_write;
		writtenSize += size_to_write;
		// If current block is full
		if(remainingSize > 0 || (remainingSize == 0 && remainingblockSize == 0))
		{
			current++;
			// Go to the next block with root's metadata
			CALL(lseek(fd,blocknums[current]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			remainingblockSize = sB.blockSize;
			size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;
		}
	}
	// Write root's datablocks (part of metadata)
	data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));			//root's data
	for(int i=0; i<(sB.maxDatablockNum); i++)
		data.datablocks[i] = 0;
	// Number of bytes to be written
	remainingSize = sB.maxDatablockNum*sizeof(unsigned int);
	// Will write as many as possible
	size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;
	writtenSize = 0;

	while(remainingSize > 0)									//write root's data
	{
		SAFE_WRITE(fd,data.datablocks,writtenSize,sizeof(unsigned int),size_to_write,sum,n,plus);

		remainingSize -= size_to_write;
		remainingblockSize -= size_to_write;
		writtenSize += size_to_write;
		// If current block is full and there are more data to be written
		if(remainingSize > 0)
		{
			current++;
			CALL(lseek(fd,blocknums[current]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			remainingblockSize = sB.blockSize;
			size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;
		}
	}
///////////////CREATE ENTITY ///////////////////////

	CALL(close(fd),-1,"Error closing file for cfs: ",4,ignore);

	free(data.datablocks);
	return fd;
}

bool cfs_close(int fileDesc) {
	if(fileDesc != -1)
	{
		int ignore = 0;
		// Update superblock, inodeTable, holeList
        	update_superBlock(fileDesc);
        	free(inodeTable);

        	CALL(close(fileDesc),-1,"Error closing file for cfs: ",4,ignore);
        	return true;
    	}

	return false;
}
