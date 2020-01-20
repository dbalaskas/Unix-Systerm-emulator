/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

superBlock	sB;
char		*inodeTable;
int		inodeSize;
List		*holes;
unsigned int	cfs_current_nodeid;

void update_superBlock(int fileDesc)
{
	int             ignore = 0, ptr_values, sum, n, plus, tableOffset = 0;
	unsigned int    new_blockNum, iblock[sB.iTableBlocksNum];

	// Write the inodeTable on the file
	int	difference = sB.iTableBlocksNum*sizeof(unsigned int);
	int	overflow_block = sB.nextSuperBlock, overflow_prev, overflow_next;
	int	size_to_read, readSize = 0;
	int	remainingblockSize = sB.blockSize - sizeof(int);

	while(difference > 0)										//read inodeTable
	{
		CALL(lseek(fileDesc,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		overflow_prev = overflow_block;
		ptr_values = 0;
		SAFE_READ(fileDesc,&overflow_block,ptr_values,sizeof(int),sizeof(int),sum,n,plus);

		size_to_read = (remainingblockSize < difference) ? remainingblockSize : difference;
		SAFE_READ(fileDesc,iblock,readSize,sizeof(unsigned int),size_to_read,sum,n,plus);

		readSize += size_to_read;
		difference -= size_to_read;
		if(difference == 0)
		{
			overflow_next = overflow_block;
			overflow_block = overflow_prev;							//block where the inodeTable ends
		}
	}

//	int size = sizeof(bool) + sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
	int size_to_write;
	int writtenSize, remainingSize;
	int freeSpaces, move;
	// For each node of the table
	for(int j=0; j<sB.iTableCounter; j++) {
        // If it has content (flag==true)
        	if (*(inodeTable+j*inodeSize) == true) {

	        	remainingSize = inodeSize-sizeof(bool);
            // if((sB.iTableBlocksNum-tableOffset)/sB.metadataBlocksNum >= 1)
            // if (sB.iTableBlocksNum-tableOffset > 0)
            // {
            //     // blocknum = &(iblock[tableOffset]);
            //     //Write on the blocks
            //     int writtenSize = 0;
            //     for(int i=0; i<sB.metadataBlocksNum; i++)
            //     {
            //         CALL(lseek(fileDesc,iblock[tableOffset+i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

            //         size_to_write = (remainingSize > sB.blockSize) ? sB.blockSize : remainingSize;
            //         sum = 0;
            //         while(sum < size_to_write)
            //         {
            //             CALL(write(fileDesc,inodeTable+j*size+sizeof(bool)+sum+writtenSize,size_to_write),-1,"Error writing from cfs file: ",2,n);
            //             sum += n;
            //         }
            //         writtenSize += size_to_write;
            //         remainingSize -= size_to_write;
            //     }

            //     tableOffset += sB.metadataBlocksNum;
            // }
            // // We assume that there no more emptyblocks left
            // else { //if(sB.nodeidCounter*sB.metadataBlocksNum > sB.iTableBlocksNum){
            //     // int newBlocks = sB.nodeidCounter*sB.metadataBlocksNum - sB.iTableBlocksNum;
                
                // int newBlocks = sB.metadataBlocksNum;
            //     for (int i=0;i<newBlocks;i++) {


			writtenSize = 0;
			//free space in block after end of inodeTable
			freeSpaces = remainingblockSize - (remainingblockSize % (sB.iTableBlocksNum*sizeof(unsigned int)));
	            	// For each of the metadata blocks that it has to allocate and write
	      	      	for(int i=0; i<sB.metadataBlocksNum; i++) {
            			// If there are still allocated blocks for the inodeTable Map
                		if (tableOffset < sB.iTableBlocksNum) {
	                		CALL(lseek(fileDesc,iblock[tableOffset]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		                // Else allocate a new block (with getEmptyBlock())
        	        	} else {
                	    		new_blockNum = getEmptyBlock();
                    			CALL(lseek(fileDesc,new_blockNum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	                	}

                		// Write on the block (fileptr is on the right spot)
                		size_to_write = (remainingSize > sB.blockSize) ? sB.blockSize : remainingSize;
				SAFE_WRITE(fileDesc,inodeTable,j*inodeSize+sizeof(bool)+writtenSize,sizeof(char),size_to_write,sum,n,plus);

        	        	writtenSize += size_to_write;
                		remainingSize -= size_to_write;

	                	// Write the new blockNum in the inodeTable Map
        	        	if (tableOffset == sB.iTableBlocksNum) {
                	    		// If there is space in the block
                    			if (freeSpaces > 0) {
						//go to end of inodeTable
						move = overflow_block*sB.blockSize + (remainingblockSize - freeSpaces);
                        			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
        	            		} else {
						// If current block has an overflow
						if(overflow_next != -1)
						{
							overflow_block = overflow_next;
							move = overflow_block*sB.blockSize;
        	                			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
							ptr_values = 0;
							SAFE_READ(fileDesc,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
						}
	                    			// Else allocate a new block for the cfs metadata
						else
						{
							overflow_block = (int) getEmptyBlock();
	                	        		move = overflow_block*sB.blockSize;

	                        			// Initialize to -1 the new pointer of the superBlock
        	                			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	                        			overflow_next = -1;
							ptr_values = 0;
							SAFE_WRITE(fileDesc,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
		                    		}

						freeSpaces = remainingblockSize;
					}

                    			// Write the blockNum of the new inode Block
					ptr_values = 0;
					SAFE_WRITE(fileDesc,&new_blockNum,ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);

                    			sB.iTableBlocksNum++;
					freeSpaces -= sizeof(unsigned int);
                		}

        	        	tableOffset++;
			}
        	}
	}
    	// Write the holeList on the file
    	unsigned int	hole;

	//as long as List is not empty, find and go to the right block to write the list
    	for (int i=0; i<sB.ListSize;i++) {
        	hole = pop(holes);
		//if there is space in current block
        	if (freeSpaces > 0) {
			move = overflow_block*sB.blockSize + (sB.blockSize - freeSpaces);
    			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		} else {
			//if current block already has an overflow
			if(overflow_next != -1)
			{
				overflow_block = overflow_next;
				move = overflow_block*sB.blockSize;
            			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				ptr_values = 0;
				SAFE_WRITE(fileDesc,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
			}
			//if an empty block is needed
			else
			{
				overflow_block = getEmptyBlock();
				move = overflow_block*sB.blockSize;
	            		CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	            		overflow_next = -1;
				ptr_values = 0;
				SAFE_WRITE(fileDesc,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
			}

			freeSpaces = remainingblockSize;
        	}
		//write a list node (fileptr is on the right spot)
		ptr_values = 0;
		SAFE_WRITE(fileDesc,&hole,ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);

		freeSpaces -= sizeof(unsigned int);
    	}

	// Write superBlock struct on the file
	CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	ptr_values = 0;
	SAFE_READ(fileDesc,&sB,ptr_values,sizeof(superBlock),sizeof(superBlock),sum,n,plus);
}

unsigned int getTableSpace()
{													//mode: SEEK_SET | SEEK_CUR | SEEK_END
/*	int		bytes, blocknum;

	CALL(lseek(fileDesc,move,mode),-1,"Error moving ptr in cfs file: ",5,bytes);
	blocknum = (int) (bytes / sB.blockSize);

	return blocknum;
*/
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

int traverse_cfs(char *filename,unsigned int start)
{
	int		offset, i;
	char		*curr_name, *split, *curr_dir = (char*)malloc(sB.filenameSize*sizeof(char));
	MDS		*metadata;
	Datastream	data;

	split = strtok(filename,"/");
	if(split == NULL)
	{
		strcpy(curr_dir,"/");
		split = curr_dir;
	}

	offset = start*inodeSize + sizeof(bool);

	while(split != NULL)
	{
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

			offset += sB.filenameSize + sizeof(MDS);
			data.datablocks = (unsigned int*) (inodeTable + offset);
			for(i=0; i<sB.maxDatablockNum; i++)
			{
				start = data.datablocks[i];
				offset = start*inodeSize + sizeof(bool);
				curr_name = inodeTable + offset;
				if(!strcmp(split,curr_name))
				{
					split = NULL;
					break;
				}
			}
			if(i == sB.maxDatablockNum)
				return -1;
		}
	}

	free(curr_dir);

	return (int) start;

/*	int		sum = 0, n, ignore = 0;
	char		*curr_name = (char*)malloc(sB.filenameSize*sizeof(char));

	CALL(lseek(fileDesc,(sB.blockSize)*start_bl,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	while(sum < sB.filenameSize)
	{
		CALL(read(fileDesc,curr_name+sum,sB.filenameSize),-1,"Error reading from cfs file: ",3,n);
		sum += n;
	}
	if(!strcmp(curr_name,filename))
	{
		free(curr_name);
		return getLocation(fileDesc,(-1)*sB.filenameSize,SEEK_CUR);
	}
	else
	{
		unsigned int	curr_block, blocknum;
		Datastream	data;

		data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));

		CALL(lseek(fileDesc,sizeof(MDS),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
		for(int i = 0; i<sB.maxDatablockNum; i++)
		{
			curr_block = data.datablocks[i];
			if(curr_block == 0)
			{
				blocknum = 0;
				break;
			}
			else
			{
				blocknum = traverse_cfs(fileDesc,filename,curr_block);
				if(blocknum)
					break;
			}
		}

		free(curr_name);
		return blocknum;
	}
*/
}

unsigned int getEmptyBlock()
{
    unsigned int new_blockNum = pop(holes);
    if(new_blockNum > 0) {
        sB.ListSize--;
        return new_blockNum;
    } else {
        sB.blockCounter++;
        return sB.blockCounter;
    }
}
