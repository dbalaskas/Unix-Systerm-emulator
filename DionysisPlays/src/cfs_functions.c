/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

void update_superBlock(cfs_info *info)
{
	int	ignore = 0, tableOffset = 0;
	int	new_blockNum, iblock[info->(sB.iTableBlocksNum)];

	// Write the inodeTable in the file
	// InodeTable's size before file was last closed
	int	difference = (info->(sB.iTableBlocksNum))*sizeof(int);
	// First block with inodeTable
	int	overflow_block = info->(sB.nextSuperBlock), overflow_prev, overflow_next;
	int	size_to_read, readSize = 0;
	// Space left in block for inodeTable and holeList
	int	remainingblockSize = info->(sB.blockSize) - sizeof(int);

	// Untill we read all elements of the old table (will be re-used for the new contents of the inodeTable)
	while(difference > 0)
	{
		CALL(lseek(info->fileDesc,overflow_block*(info->(sB.blockSize)),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		overflow_prev = overflow_block;
		SAFE_READ(info->fileDesc,&overflow_block,0,sizeof(int),sizeof(int));
		// Read as many bytes as needed/possible
		size_to_read = (remainingblockSize < difference) ? remainingblockSize : difference;
		SAFE_READ(info->fileDesc,iblock,readSize,sizeof(int),size_to_read);

		readSize += size_to_read;
		difference -= size_to_read;
		// If (old) inodeTable ends in current block
		if(difference == 0)
		{
			overflow_next = overflow_block;
			overflow_block = overflow_prev;
		}
	}

	int		size_to_write;
	int		writtenSize, writtenData;
	int		freeSpaces, move;
	int		dataSize;
	MDS		*metadata;
	Datastream	data;

	// Number of blocks used to write metadata
	int		usedBlocks = 0;

	// For every element of the inodeTable
	for(int j=0; j<(info->(sB.iTableCounter)); j++) {
	        // If it has content (flag==true)
        	if (*(bool*)(info->inodeTable+j*(info->inodeSize)) == true) {
			// Free space in block after end of inodeTable
			freeSpaces = remainingblockSize - (((info->(sB.iTableBlocksNum))*sizeof(int)) % remainingblockSize);

			// If there are still allocated blocks for the inodeTable Map
                	if (tableOffset < (info->(sB.iTableBlocksNum))) {
				move = iblock[tableOffset]*(info->(sB.blockSize));
	                	CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		        // Else allocate a new block (with getEmptyBlock())
        	        } else {
                		new_blockNum = getEmptyBlock();
				move = new_blockNum*(info->(sB.blockSize));
                    		CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	                }

                	// Write on the block (fileptr is on the right spot)
			// Will write filename and MDS struct for now (stored easily in only one block)
			size_to_write = info->(sB.filenameSize) + sizeof(MDS);
			SAFE_WRITE(info->fileDesc,info->inodeTable,j*(info->inodeSize)+sizeof(bool),sizeof(char),size_to_write);

        	        writtenSize = size_to_write;
			// Get datablocksCounter (number of datablocks) from metadata
			metadata = (MDS*) (info->inodeTable + j*(info->inodeSize) + sizeof(bool) + info->(sB.filenameSize));
			dataSize = (metadata->datablocksCounter)*sizeof(int);
			// Get datablocks so as to check which of them are empty
			data.datablocks = (int*) (info->inodeTable+j*(info->inodeSize)+ sizeof(bool) + info->(sB.filenameSize) + sizeof(MDS));

			writtenData = 0;
			// Write all datablocks with contents (to the appropriate blocks)
			size_to_write = (dataSize > ((info->(sB.blockSize))-writtenSize)) ? ((info->(sB.blockSize))-writtenSize) : (dataSize);
			// size_to_write must be a multiple of sizeof(int)
			if(size_to_write % sizeof(int))
				size_to_write -= size_to_write % sizeof(int);
//			while(dataSize > 0)
			do {
				// Write as many (non-empty) datablocks as possible in current block
				while(writtenData*sizeof(int) < size_to_write)
				{
					move = j*(info->inodeSize) + sizeof(bool) + writtenSize + writtenData*sizeof(int);
					if(data.datablocks[writtenData] != -1)
					{
						SAFE_WRITE(info->fileDesc,info->inodeTable,move,sizeof(char),sizeof(int));
						writtenData++;
					}
				}
				dataSize -= size_to_write;
				writtenSize += size_to_write;

				// Write the new blockNum in the inodeTable Map
        		        if (tableOffset == (info->(sB.iTableBlocksNum))) {
                			// If there is space in the block
                    			if (freeSpaces > 0) {
						// Go to end of inodeTable
						move = overflow_block*(info->(sB.blockSize)) + ((info->(sB.blockSize)) - freeSpaces);
	                        		CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
        		            	} else {
						// If current block has an overflow
						if(overflow_next != -1)
						{
							overflow_block = overflow_next;
							move = overflow_block*(info->(sB.blockSize));
	        	                		CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
							SAFE_READ(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
						}
	                	    		// Else allocate a new block for the cfs metadata
						else
						{
							overflow_block = getEmptyBlock();
		                	       		move = overflow_block*(info->(sB.blockSize));

							// Initialize to -1 the new pointer of the superBlock
       		                			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

                	        			overflow_next = -1;
							SAFE_WRITE(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
	                    			}

						freeSpaces = remainingblockSize;
					}

	                    		// Write the blockNum of the new inode Block
					SAFE_WRITE(info->fileDesc,&new_blockNum,0,sizeof(int),sizeof(int));

                	    		(info->(sB.iTableBlocksNum))++;
					freeSpaces -= sizeof(int);
                		}

				// Go to the next element in inodeTable
	        	       	tableOffset++;

				// If there are more datablocknums to be written
				if(dataSize > 0)
				{
					// If there are still allocated blocks for the inodeTable Map
		                	if (tableOffset < (info->(sB.iTableBlocksNum))) {
						move = iblock[tableOffset]*(info->(sB.blockSize));
	        	        		CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		        		// Else allocate a new block (with getEmptyBlock())
		        	        } else {
                				new_blockNum = getEmptyBlock();
						move = new_blockNum*(info->(sB.blockSize));
		                    		CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	        		        }

					writtenData = 0;
					size_to_write = (dataSize > ((info->(sB.blockSize))-writtenSize)) ? ((info->(sB.blockSize))-writtenSize) : (dataSize);
					// size_to_write must be a multiple of sizeof(int)
					if(size_to_write % sizeof(int))
						size_to_write -= size_to_write % sizeof(int);
				}
				// Another block was used for metadata
				usedBlocks++;
			} while(dataSize > 0);
	      	}
	}
	// If the blocks used for metadata are less than the ones before (after previous cfs close)
	if(usedBlocks < (info->(sB.iTableBlocksNum)))
	{	// Add all useless blocks in holeList
		for(int i=0; i<((info->(sB.iTableBlocksNum))-usedBlocks); i++)
		{
			addNode(&(info->holes),iblock[usedBlocks+i]);
			// Increase list's size
			(info->(sB.ListSize))++;
			// Increase free space in overflow block by an int
			freeSpaces += sizeof(int);
		}
		// Decrease number of blocks needed for the iTable
		info->(sB.iTableBlocksNum) = usedBlocks;
	}

	// If current overflow block has its own overflow, check if it is needed to store the holeList
	if(overflow_next != -1)
	{
		int	empty_overflow = overflow_next;
		// Space left for the List in current overflow block
		int	currentSpace = freeSpaces;
		// List's size in bytes
		int	listNodes = (info->(sB.ListSize))*sizeof(int);
		int	next;

		// While there is another overflow block
		while(empty_overflow != -1)
		{
			// If there's enough space in current overflow block for the List (+next overflow block that will be added in List)
			if((listNodes + sizeof(int)) <= currentSpace)
			{	// Next overflow block is not needed after all (iTable + List require less overflow blocks than last time)
				// Add it to holeList
				addNode(&(info->holes),empty_overflow);
				(info->(sB.ListSize))++;
				// Increase number of bytes required to store the List
				listNodes += sizeof(int);
				// Get next overflow block's number
				move = empty_overflow*(info->(sB.blockSize));
			    	CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(info->fileDesc,&next,0,sizeof(int),sizeof(int));
				// If current overflow block is full or there is no other overflow block after it
				if(currentSpace <= 0 || next == -1)
					empty_overflow = next;
			}
			// Next overflow block is needed
			else
			{	// Assume as many listNodes as possible will be stored in current overflow block
				listNodes -= currentSpace;
				// New currentSpace is free space in the next overflow block (which will be used)
				currentSpace = (info->(sB.blockSize)) - sizeof(int);
				// Check for another (possibly useless) overflow block
				move = empty_overflow*(info->(sB.blockSize));
				CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(info->fileDesc,&empty_overflow,0,sizeof(int),sizeof(int));
			}
		}
	}

	// Write the holeList on the file
    	int	hole;
	// As long as List is not empty, find and go to the right block to write the list
    	for (int i=0; i<(info->(sB.ListSize));i++) {
        	hole = pop_minimum_Node(&(info->holes));
		// If there is space in current block
        	if (freeSpaces > 0) {
				move = overflow_block*(info->(sB.blockSize)) + ((info->(sB.blockSize)) - freeSpaces);
    			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			} else {
				// If current block already has an overflow
				if(overflow_next != -1)
				{
					overflow_block = overflow_next;
					move = overflow_block*(info->(sB.blockSize));
					CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_WRITE(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
				}
				// If an empty block is needed
				else
				{
					overflow_block = getEmptyBlock();
					move = overflow_block*(info->(sB.blockSize));
					CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					overflow_next = -1;
					SAFE_WRITE(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
				}

				freeSpaces = remainingblockSize;
        	}
		// Write a list node (fileptr is on the right spot)
		SAFE_WRITE(info->fileDesc,&hole,0,sizeof(int),sizeof(int));
		freeSpaces -= sizeof(int);
    	}

	// Write superBlock struct on the file
	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(info->sB),0,sizeof(superBlock),sizeof(superBlock));
}

// Get (first) empty space in inodeTable
int getTableSpace()
{
	bool	busy;
	int	i;

	for(i=0; i<sB.iTableCounter; i++)
	{
		busy = *(bool*) (inodeTable + i*inodeSize);
		if(busy == false)
			break;
	}
	return i;
}

// Find entity (filname) in cfs, starting from entity with nodeid 'start'
int traverse_cfs(cfs_info *info,char *filename,int start)
{
	int		offset, i, move;
	int		blocknum, datablocks_checked, dataCounter, j, ignore = 0;
	char		*split;
	char		path_name[sB.filenameSize];
	char		*curr_name = (char*)malloc(sB.filenameSize*sizeof(char));
	char		root[2];
	MDS		*metadata, *parent_mds;
	Datastream	data;

	// 'path_name' will be used by strtok, so that 'filename' remains unchanged
	strcpy(path_name,filename);
	// Get first entity in path
	split = strtok(path_name,"/");
	// If current entity is the root
	if(split == NULL)
	{
		strcpy(root,"/");
		split = root;
	}

	// Go to the element of the inodeTable with nodeid equal to 'start' (path's first entity)
	offset = start*inodeSize + sizeof(bool);
	// As far as there is another entity in path
	while(split != NULL)
	{
		// If current entity is "."
		if(!strcmp(split,"."))
			split = inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool);
		// If current entity is ".."
		else if(!strcmp(split,".."))
		{
			metadata = (MDS*) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			split = inodeTable + (metadata->parent_nodeid)*inodeSize + sizeof(bool);
		}
		// Go to  entity's metadata and data
		offset += sB.filenameSize;
		metadata = (MDS*) (inodeTable + offset);
		offset += sizeof(MDS);
		data.datablocks = (int*) (inodeTable + offset);
		// If it is a directory
		if(metadata->type == Directory)
		{
			// Number of datablocks we have searched
			datablocks_checked = 0;
			// For directory's each data block
			for(i=0; i<sB.maxFileDatablockNum; i++)
			{
				// If it has contents
				if(data.datablocks[i] != -1)
				{
					// Check directory's contents
					blocknum = data.datablocks[i];
					CALL(lseek(fd,blocknum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(fd,&dataCounter,0,sizeof(int),sizeof(int));

					// For every pair of (filename+nodeid) in datablock
					for(j=0; j<dataCounter; j++)
					{
						move = blocknum*sB.blockSize + sizeof(int) + j*(sB.filenameSize+sizeof(int));
						CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(fd,curr_name,0,sizeof(char),sB.filenameSize);
						if(!strcmp(curr_name,"."))
							strcpy(curr_name, inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool));
						else if(!strcmp(curr_name,".."))
						{
							parent_mds=(MDS*)(inodeTable+cfs_current_nodeid*inodeSize+sizeof(bool)+sB.filenameSize);
							strcpy(curr_name, inodeTable + (parent_mds->parent_nodeid)*inodeSize + sizeof(bool));
						}

						// If current directory contains the entity we are looking for
						if(!strcmp(split,curr_name))
						{
							// Keep entity's place in inodeTable
							SAFE_READ(fd,&start,0,sizeof(int),sizeof(int));
							offset = start*inodeSize + sizeof(bool);
							break;
						}
					}
					// If entity was found
					if(j < dataCounter)
						break;
					// Go to the next datablock
					datablocks_checked++;
				}
				// If there are no more datablocks with contents
				if(datablocks_checked == metadata->datablocksCounter)
					break;
			}
			// If entity wasn't found
			if(i == sB.maxFileDatablockNum || datablocks_checked == metadata->datablocksCounter)
			{
				free(curr_name);
				return -1;
			}
		}
		// If current entity is a file
		else if(metadata->type == File)
		{
			split = strtok(NULL,"/");
			// But there is another entity in path
			if(split != NULL)
			{
				free(curr_name);
				return -1;
			}
			// Else, it is the file we were looking for ('start' has already been appropriately updated)
			break;
		}

		// Get next entity's name in path
		split = strtok(NULL,"/");
	}

	free(curr_name);
	return start;
}

int getEmptyBlock()
{
    int new_blockNum = pop_minimum_Node(&holes);
    if(new_blockNum != -1) {
        sB.ListSize--;
        return new_blockNum;
    } else {
        sB.blockCounter++;
        return sB.blockCounter;
    }
}

int getPathStartId(char* path)
{
	int start;
	char initial_path[strlen(path)+1];
	if(path[0] == '/')
	{
		// Traversing cfs will start from the root (nodeid = 0)
		start = 0;
	}
	else
	{
		// Get first entity in path
		strcpy(initial_path,path);
		char *start_dir = strtok(initial_path,"/");
		// If path starts from current directory's parent
		if(strcmp(start_dir,"..") == 0)
		{
			MDS *current_mds = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			start = current_mds->parent_nodeid;
		}
		else	//if(!strcmp(split,".") || strcmp(split,"/"))
		{
			// If path starts from current directory
			start = cfs_current_nodeid;
		}
	}

	return start;
}

void replaceEntity(cfs_info *info, int source_nodeid, int destination_nodeid)
{	
	MDS *source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	int *source_data = (int*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	int source_dataBlocksNum = source_mds->datablocksCounter;
	MDS *destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	int *destination_data = (int*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	int destination_dataBlocksNum = destination_mds->datablocksCounter;

	int ignore;
	int j = 0;
	bool blocksExpired = false;
	int blockNum;
	char buff[sB.blockSize];
	int blocksUsed = 0;
	for (int i=0; i < sB.maxFileDatablockNum; i++) {
		if (source_data[i] != -1) {
			if (blocksExpired == false) {
				while (destination_data[j] == -1)
					j++;
				blockNum = destination_data[j];
				j++;
				blocksUsed++;
				if (blocksUsed == destination_dataBlocksNum){
					blocksExpired = true;
					j=0;
				}
			} else {
				blockNum = getEmptyBlock();
				// j = 0;
				while (destination_data[j] != -1)
					j++;
				destination_data[j] = blockNum;
				
			}
			
			CALL(lseek(info->fileDesc,source_data[i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,buff,0,sizeof(char),sB.blockSize);

			CALL(lseek(info->fileDesc,blockNum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(info->fileDesc,buff,0,sizeof(char),sB.blockSize);
		}
		if (blocksUsed == source_dataBlocksNum)
			break;
	}
	if (blocksExpired == false) {
		for(;j<sB.maxFileDatablockNum;j++) {
			destination_data[j] = -1;
		}
	}
	destination_mds->datablocksCounter = source_mds->datablocksCounter;
	destination_mds->size = source_mds->size;
}

int get_parent(cfs_info *info, char *path,char *new_name)
{
	int		ignore = 0;
	int		start;
	int		parent_nodeid;
	char		*split, *temp;
 	char		*parent_name, initial_path[strlen(path)+1];
	int 		parent_name_Size;
	MDS		*current_mds;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	strcpy(initial_path,path);
	// If path starts with "/"
	if(!strncmp(path,"/",1))
	{
		// Parent path will also start with "/"
		parent_name = malloc(2);
		strcpy(parent_name, "/");
		// Traversing cfs will start from the root (nodeid = 0)
		start = 0;
		// Get next entity in path
		split = strtok(initial_path,"/");
	}
	else
	{
		// Get first entity in path
		split = strtok(initial_path,"/");
		// If path starts from current directory's parent
		if(!strcmp(split,".."))
		{
			current_mds = (MDS*)(inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			start = current_mds->parent_nodeid;
			parent_name = malloc(4);
			strcpy(parent_name, "../");
			split = strtok(NULL,"/");
		}
		else	//if(!strcmp(split,".") || strcmp(split,"/"))
		{
			// If path starts from current directory
			start = cfs_current_nodeid;
			parent_name = malloc(3);
			strcpy(parent_name, "./");
			if(!strcmp(split,"."))
				// Get next entity in path
				split = strtok(NULL,"/");
		}
	}

	// If root is given as new name
	if (split == NULL) {
		printf("Input error, root was given as new entity.\n");
		free(parent_name);
		return -1;
	}

	// Get parent's name and new entity's name
	temp = strtok(NULL,"/");
	while(temp != NULL)
	{
		parent_name_Size = strlen(parent_name) + strlen(split) + 1;
		parent_name = realloc(parent_name, parent_name_Size);
		strcat(parent_name, split);

		split = temp;
		temp = strtok(NULL,"/");
		if(temp != NULL) {
			parent_name_Size = strlen(parent_name) + 2;
			parent_name = realloc(parent_name, parent_name_Size);
			strcat(parent_name,"/");
		}
	}

	// If new entity's name is too long
	if(strlen(split)+1 > sB.filenameSize) {
		printf("Input error, too long directory name.\n");
		free(parent_name);
		return -1;
	}

	// If the name from the path is asked
	if(new_name != NULL)
		strcpy(new_name,split);
	// Find parent directoy in cfs (meaning in inodeTable)
	parent_nodeid = traverse_cfs(fd,parent_name,start);
	if(parent_nodeid == -1)
	{
		printf("Path %s could not be found in cfs.\n",parent_name);
		free(parent_name);
		return -1;
	}
	free(parent_name);

	return parent_nodeid;
}

void append_file(cfs_info *info,int source_nodeid,int output_nodeid)
{
	int		ignore = 0;
	int		source_block, output_block;
	int		source_dataCounter, output_dataCounter;
	char		buffer[sB.blockSize];
	MDS		*source_mds, *output_mds;
	Datastream	source_data, output_data;

	source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	source_dataCounter = source_mds->datablocksCounter;
	source_data.datablocks = (int*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	output_mds = (MDS*) (inodeTable + output_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	output_dataCounter = output_mds->datablocksCounter;
	output_data.datablocks = (int*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

	for(int i=0; i<source_dataCounter; i++)
	{
		// Read i-th block from sourch file
		source_block = source_data.datablocks[i];
		CALL(lseek(fd,source_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		SAFE_READ(fd,buffer,0,sizeof(char),sB.blockSize);
		// Write it to the respective block of the output file
		output_block = getEmptyBlock();
		output_data.datablocks[output_dataCounter+i] = output_block;
		CALL(lseek(fd,output_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		SAFE_WRITE(fd,buffer,0,sizeof(char),sB.blockSize);
	}

	output_mds->datablocksCounter += source_dataCounter;
	output_mds->size += source_mds->size;
}

int getDirEntities(cfs_info *info, char *directory_path, string_List **content)
{
	if (directory_path == NULL)
		return -1;
	int 	 ignore;
	int 	 directory_nodeid = traverse_cfs(fd, directory_path, getPathStartId(directory_path));
	int 	*dir_data = (int*) (inodeTable + directory_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

	char 	 fileName[sB.filenameSize];
	char	 path[strlen(directory_path) + sB.filenameSize + 2];
	int 	 contentCounter = 0;
	int 	 pairCounter;
	for (int i=0; i < sB.maxFileDatablockNum; i++) {
		if (dir_data[i] != -1) {
			CALL(lseek(fd,dir_data[i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(fd,&pairCounter,0,sizeof(int),sizeof(int));
			if(content != NULL) {
				for (int k=0; k < pairCounter; k++) {
					CALL(lseek(fd,dir_data[i]*sB.blockSize + sizeof(int) + k*(sB.filenameSize+sizeof(int)),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(fd,fileName,0,sizeof(char),sB.filenameSize);
					if(strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0) {
						strcpy(path, directory_path);
						strcat(path, "/");
						strcat(path, fileName);
						add_stringNode(content, path);
					}
				}
			}
			contentCounter += pairCounter;
		}
	}
	return contentCounter;
}

void print_data(cfs_info *info,char *path)
{
	int		ignore = 0, move;

	// To print the superblock
	if(!strcmp(path,"superblock"))
	{
		printf("nextSuperBlock: %d\n"
			"root: %d\n"
			"blockCounter: %d\n"
			"nodeidCounter: %d \n"
			"listSize: %d\n"
			"iTableBlocksNum: %d\n"
			"iTableCounter: %d\n",sB.nextSuperBlock,sB.root,sB.blockCounter,sB.nodeidCounter,sB.ListSize,sB.iTableBlocksNum,sB.iTableCounter);
	}
	// To print the overflow blocks, or the metadata blocks
	else if(!strcmp(path,"overflow") || !strcmp(path,"metadata"))
	{
		// Whether we are reading blocks of the holeList or not
		bool	inList = false;
		int	overflow_prev, overflow = sB.nextSuperBlock;
		// Blocknum in iTable map or holeList, and number of blocknums to be read
		int	content, nums_to_read;
		// Number of blocknums that have been read, and the number of blocknums left to read (for the iTable at first)
		int	readNums, rest = sB.iTableBlocksNum;
		// Max number of blocknums that can be in an overflow block
		int	numsPerBlock = (sB.blockSize - sizeof(int)) / sizeof(int);
		if((sB.blockSize-sizeof(int)) % sizeof(int))
			numsPerBlock--;

		// As long as there are overflow blocks
		while(overflow != -1)
		{	// If user asked to print the overflow blocks, print current overflow blocknum
			if(!strcmp(path,"overflow"))
				printf("OverflowBlock: %d\n",overflow);
			// Keep current overflow blocknum and get the next overflow block
			overflow_prev = overflow;
			CALL(lseek(info->fileDesc,overflow*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,&overflow,0,sizeof(int),sizeof(int));
			// While th number of blocknums that have been read in current overflow block dont exceed the maximum
			readNums = 0;
			while(readNums < numsPerBlock)
			{	// If there are still blocknums to read for the iTable map, read as many as possible
				if(rest > 0 && !inList)
					nums_to_read = (rest > numsPerBlock) ? numsPerBlock : rest;
				// If there are no more blocknums for the iTable map, will read blocknums for the holeList
				else if(!inList)
				{	// Number of blocknums to read for the holeList
					rest = sB.ListSize;
					inList = true;
				}
				// If we are reading blocknums for the List, read as many as possible
				if(inList)
					nums_to_read = ((rest+readNums) > numsPerBlock) ? (numsPerBlock-readNums) : rest;
				// For each blocknum that is to be read from the overflow block
				for(int i=readNums; i<(readNums+nums_to_read); i++)
				{	// Go to current blocknum
					move = overflow_prev*sB.blockSize + sizeof(int) + i*sizeof(int);
					CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(info->fileDesc,&content,0,sizeof(int),sizeof(int));
					// If user asked to print the overflow block, print current's contents
					if(!strcmp(path,"overflow"))
						printf("%d\t",content);
					// If user asked to print the metadata blocks, print current's contents
					else
					{
						char	content_name[sB.filenameSize];
						MDS	content_mds;
						// Print metadata block's blocknum
						printf("MetadataBlock: %d\n",content);
						// Go to the metadata block read from the overflow
						move = content*sB.blockSize;
						CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(info->fileDesc,content_name,0,sizeof(char),sB.filenameSize);
						SAFE_READ(info->fileDesc,&content_mds,0,sizeof(MDS),sizeof(MDS));

						int	datablocks[content_mds.datablocksCounter];
						SAFE_READ(info->fileDesc,datablocks,0,sizeof(int),content_mds.datablocksCounter*sizeof(int));
						// Print the filename, datablock-counter and the numbers of the datablocks
						printf("%s\tdatablocksCounter: %d\n",content_name,content_mds.datablocksCounter);
						if(content_mds.datablocksCounter > 0)
							printf("Datablocks: ");
						for(int j=0; j<(content_mds.datablocksCounter); j++)
							printf("%d\t",datablocks[j]);
						printf("\n-------------------------------------------------------------------------------\n");
					}
				}
				if(!strcmp(path,"overflow"))
					printf("\n");
				// Increase number of blocknums that have been read
				readNums += nums_to_read;
				// Decrease number of blocknums left to read
				rest -= nums_to_read;
				// If all blocknums are read from the last overflow block
				if(rest == 0 && overflow == -1)
					break;
			}
		}
	}
	// If user asked to print a cfs entity
	else
	{
		int		start, nodeid, dataCounter;
		int		checked = 0;
		MDS		*metadata;
		Datastream	data;

		// Get entity's nodeid
		start = getPathStartId(path);
		nodeid = traverse_cfs(info->fileDesc,path,start);
		// If path was invalid
		if(nodeid == -1)
			printf("Path %s could not be found in cfs.\n",path);

		else
		{	// Get entity's metadata and data
			metadata = (MDS*) (inodeTable + nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			data.datablocks = (int*) (inodeTable + nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
			// Print entity's name and nodeid
			printf("%s->%d\n",inodeTable+nodeid*inodeSize+sizeof(bool),nodeid);
			printf("type: %d\tsize: %d\tparent: %d\tdatablocksCounter: %d\n",metadata->type,metadata->size,metadata->parent_nodeid,metadata->datablocksCounter);

			int	content_nodeid;
			char	content_name[sB.filenameSize];
			// For each datablock
			for(int i=0; i<sB.maxFileDatablockNum; i++)
			{	// If current datablock is not empty
				if(data.datablocks[i] != -1)
				{	// If it is a directory
					if(metadata->type == Directory)
					{	// Go to the appropriate datablock and print its contents
						CALL(lseek(info->fileDesc,data.datablocks[i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
						printf("Datablock %d: counter = %d\n",data.datablocks[i],dataCounter);
						// For each entity in the directory, print its name and nodeid
						for(int j=0; j<dataCounter; j++)
						{
							SAFE_READ(fileDesc,content_name,0,sizeof(char),sB.filenameSize);
							SAFE_READ(fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));
							printf("%s->%d\t",content_name,content_nodeid);
						}
						printf("\n");
					}
					// If it is a file, print the number of the datablock
					else
						printf("Datablock %d\n",data.datablocks[i]);

					checked++;
				}
				// If all datablocks are printed
				if(checked == metadata->datablocksCounter)
					break;
			}
		}
	}
}
