/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

superBlock	sB;
char		*inodeTable;
int		inodeSize;
List		*holes;
int		cfs_current_nodeid;

void update_superBlock(int fileDesc)
{
	int	ignore = 0, sum, n, plus, tableOffset = 0;
	int	new_blockNum, iblock[sB.iTableBlocksNum];

	// Write the inodeTable in the file
	// InodeTable's size before file was last closed
	int	difference = sB.iTableBlocksNum*sizeof(int);
	// First block with inodeTable
	int	overflow_block = sB.nextSuperBlock, overflow_prev, overflow_next;
	int	size_to_read, readSize = 0;
	// Space left in block for inodeTable and holeList
	int	remainingblockSize = sB.blockSize - sizeof(int);

	// Untill we read all elements of the old table (will be re-used for the new contents of the inodeTable)
	while(difference > 0)
	{
		CALL(lseek(fileDesc,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		overflow_prev = overflow_block;
		SAFE_READ(fileDesc,&overflow_block,0,sizeof(int),sizeof(int),sum,n,plus);
		// Read as many bytes as needed/possible
		size_to_read = (remainingblockSize < difference) ? remainingblockSize : difference;
		SAFE_READ(fileDesc,iblock,readSize,sizeof(int),size_to_read,sum,n,plus);

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

	// For every element of the inodeTable
	for(int j=0; j<sB.iTableCounter; j++) {
	        // If it has content (flag==true)
        	if (*(bool*)(inodeTable+j*inodeSize) == true) {
			// Free space in block after end of inodeTable
			freeSpaces = remainingblockSize - ((sB.iTableBlocksNum*sizeof(int)) % remainingblockSize);

			// If there are still allocated blocks for the inodeTable Map
                	if (tableOffset < sB.iTableBlocksNum) {
	                	CALL(lseek(fileDesc,iblock[tableOffset]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		        // Else allocate a new block (with getEmptyBlock())
        	        } else {
                		new_blockNum = getEmptyBlock();
                    		CALL(lseek(fileDesc,new_blockNum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	                }

                	// Write on the block (fileptr is on the right spot)
			// Will write filename and MDS struct for now (stored easily in only one block)
			size_to_write = sB.filenameSize + sizeof(MDS);
			SAFE_WRITE(fileDesc,inodeTable,j*inodeSize+sizeof(bool),sizeof(char),size_to_write,sum,n,plus);

        	        writtenSize = size_to_write;
			// Get datablocksCounter (number of datablocks) from metadata
			metadata = (MDS*) (inodeTable + j*inodeSize + sizeof(bool) + sB.filenameSize);
			dataSize = (metadata->datablocksCounter)*sizeof(int);
			// Get datablocks so as to check which of them are empty
			data.datablocks = (int*) (inodeTable + j*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

			writtenData = 0;
			// Write all datablocks with contents (to the appropriate blocks)
			size_to_write = (dataSize > (sB.blockSize-writtenSize)) ? (sB.blockSize-writtenSize) : (dataSize);
			// size_to_write must be a multiple of sizeof(int)
			if(size_to_write % sizeof(int))
				size_to_write -= size_to_write % sizeof(int);
			while(dataSize > 0)
			{
				// Write as many (non-empty) datablocks as possible in current block
				while(writtenData*sizeof(int) < size_to_write)
				{
					move = j*inodeSize + sizeof(bool) + writtenSize + writtenData*sizeof(int);
					if(data.datablocks[writtenData] != -1)
					{
						SAFE_WRITE(fileDesc,inodeTable,move,sizeof(char),sizeof(int),sum,n,plus);
						writtenData++;
					}
				}
				dataSize -= size_to_write;
				writtenSize += size_to_write;

				// Write the new blockNum in the inodeTable Map
        		        if (tableOffset == sB.iTableBlocksNum) {
                			// If there is space in the block
                    			if (freeSpaces > 0) {
						// Go to end of inodeTable
						move = overflow_block*sB.blockSize + (sB.blockSize - freeSpaces);
	                        		CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
        		            	} else {
						// If current block has an overflow
						if(overflow_next != -1)
						{
							overflow_block = overflow_next;
							move = overflow_block*sB.blockSize;
	        	                		CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
							SAFE_READ(fileDesc,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
						}
	                	    		// Else allocate a new block for the cfs metadata
						else
						{
							overflow_block = getEmptyBlock();
		                	       		move = overflow_block*sB.blockSize;

							// Initialize to -1 the new pointer of the superBlock
       		                			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

                	        			overflow_next = -1;
							SAFE_WRITE(fileDesc,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
	                    			}

						freeSpaces = remainingblockSize;
					}

	                    		// Write the blockNum of the new inode Block
					SAFE_WRITE(fileDesc,&new_blockNum,0,sizeof(int),sizeof(int),sum,n,plus);

                	    		sB.iTableBlocksNum++;
					freeSpaces -= sizeof(int);
                		}

				// Go to the next element in inodeTable
	        	       	tableOffset++;

				// If there are more datablocknums to be written
				if(dataSize > 0)
				{
					// If there are still allocated blocks for the inodeTable Map
		                	if (tableOffset < sB.iTableBlocksNum) {
	        	        		CALL(lseek(fileDesc,iblock[tableOffset]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		        		// Else allocate a new block (with getEmptyBlock())
		        	        } else {
                				new_blockNum = getEmptyBlock();
		                    		CALL(lseek(fileDesc,new_blockNum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	        		        }

					writtenData = 0;
					size_to_write = (dataSize > (sB.blockSize-writtenSize)) ? (sB.blockSize-writtenSize) : (dataSize);
					// size_to_write must be a multiple of sizeof(int)
					if(size_to_write % sizeof(int))
						size_to_write -= size_to_write % sizeof(int);
				}
			}
        	}
	}
    	// Write the holeList on the file
    	int	hole;

	// As long as List is not empty, find and go to the right block to write the list
    	for (int i=0; i<sB.ListSize;i++) {
        	hole = pop(holes);
		// If there is space in current block
        	if (freeSpaces > 0) {
			move = overflow_block*sB.blockSize + (sB.blockSize - freeSpaces);
    			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		} else {
			// If current block already has an overflow
			if(overflow_next != -1)
			{
				overflow_block = overflow_next;
				move = overflow_block*sB.blockSize;
            			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(fileDesc,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
			}
			// If an empty block is needed
			else
			{
				overflow_block = getEmptyBlock();
				move = overflow_block*sB.blockSize;
	            		CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	            		overflow_next = -1;
				SAFE_WRITE(fileDesc,&overflow_next,0,sizeof(int),sizeof(int),sum,n,plus);
			}

			freeSpaces = remainingblockSize;
        	}
		// Write a list node (fileptr is on the right spot)
		SAFE_WRITE(fileDesc,&hole,0,sizeof(int),sizeof(int),sum,n,plus);

		freeSpaces -= sizeof(int);
    	}

	// Write superBlock struct on the file
	CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fileDesc,&sB,0,sizeof(superBlock),sizeof(superBlock),sum,n,plus);
}

// Get (first) empty space in inodeTable
int getTableSpace()
{
	bool	busy;
	int	i;

	for(i=0; i<sB.iTableCounter; i++)
	{
		busy = (bool) (inodeTable + i*inodeSize);
		if(busy == false)
			break;
	}

	return i;
}

// Find entity (filname) in cfs, starting from entity with nodeid 'start'
int traverse_cfs(char *filename,int start)
{
	int		offset, i;
	char		*curr_name, *split, *curr_dir = (char*)malloc(sB.filenameSize*sizeof(char));
	MDS		*metadata;
	Datastream	data;

	split = strtok(filename,"/");
	// If the entity we are looking for is the root
	if(split == NULL)
	{
		strcpy(curr_dir,"/");
		split = curr_dir;
	}
	// Go to the element of the inodeTable with nodeid equal to 'start'
	offset = start*inodeSize + sizeof(bool);
	// As far as there is another entity in path
	while(split != NULL)
	{
		// Get next entity's name in path
		split = strtok(NULL,"/");
		if(split != NULL)
		{
			if(!strcmp(split,"."))
				split = inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool);
			else if(!strcmp(split,".."))
			{
				metadata = (MDS*) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				split = inodeTable + (metadata->parent_nodeid)*inodeSize + sizeof(bool);
			}
			// Go to next entity's data (has to be a directory)
			offset += sB.filenameSize + sizeof(MDS);
			data.datablocks = (int*) (inodeTable + offset);
			for(i=0; i<sB.maxDatablockNum; i++)
			{
				// Check directory's contents
				start = data.datablocks[i];
				offset = start*inodeSize + sizeof(bool);
				curr_name = inodeTable + offset;
				if(!strcmp(split,curr_name))
				{
					split = NULL;
					break;
				}
			}
			// If entity wasn't found
			if(i == sB.maxDatablockNum)
				return -1;
		}
	}

	free(curr_dir);

	return (int) start;
}

int getEmptyBlock()
{
    int new_blockNum = pop(holes);
    if(new_blockNum > 0) {
        sB.ListSize--;
        return new_blockNum;
    } else {
        sB.blockCounter++;
        return sB.blockCounter;
    }
}
