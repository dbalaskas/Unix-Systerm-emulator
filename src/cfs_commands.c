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

int cfs_workwith(char *filename,bool open_cfs)
{
	int             fd = -1, ignore = 0, ptr_values, sum, n, plus;
	int		offset = 0, listOffset;
	unsigned int	current_iblock[sB.metadataBlocksNum];

	if(open_cfs == true)
	{
        	perror("Error: a cfs file is already open. Close it if you want to open other cfs file.\n");
	}
	else
	{
		CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);
		CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
//		sum = 0;
//        	while(sum < sizeof(superBlock))
//		{
//			plus = sum / sizeof(superBlock);
//			CALL(read(fd,(&sB)+plus,sizeof(superBlock)),-1,"Error reading from cfs file: ",2,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_READ(fd,&sB,ptr_values,sizeof(superBlock),sizeof(superBlock),sum,n,plus);

		CALL(lseek(fd,(sB.nextSuperBlock*sB.blockSize)+sizeof(int),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		//go to nextSuperBlock (with iTable and List info), after the overflow int

		int size = sizeof(bool) + sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
        	char buffer[size-sizeof(bool)];
	        int remainingSize, size_to_read;
	        int *nodeId;
	        inodeTable = (char*)malloc(sB.iTableCounter*sizeof(char)*size);
	        holes = NULL;
	        int numOfiNode = 0;
        
	        for(int i=0; i<sB.nodeidCounter; i++)
	        {
//			sum = 0;
//	            	while(sum < sB.metadataBlocksNum*sizeof(unsigned int))
//			{
//				plus = sum / sizeof(unsigned int);
  //              		CALL(read(fd,current_iblock+plus,sB.metadataBlocksNum*sizeof(unsigned int)),-1,"Error reading from cfs file: ",2,n);
//	                	sum += n;
//	            	}
			ptr_values = 0;
			SAFE_READ(fd,current_iblock,ptr_values,sizeof(unsigned int),sB.metadataBlocksNum*sizeof(unsigned int),sum,n,plus);

			remainingSize = size-sizeof(bool);
            		for(int j=0; j<sB.metadataBlocksNum; j++)
	            	{
        	        	CALL(lseek(fd,current_iblock[j]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

                		size_to_read = (remainingSize > sB.blockSize) ? sB.blockSize : remainingSize;
//                		sum = 0;
//	                	while(sum < size_to_read)
//        	        	{
//                	    		CALL(read(fd,buffer+j*sB.blockSize+sum,size_to_read),-1,"Error reading from cfs file: ",2,n);
//                    			sum += n;
//	                	}
				SAFE_READ(fd,buffer,j*sB.blockSize,sizeof(char),size_to_read,sum,n,plus);

        	        	remainingSize -= size_to_read;
            		}

	            	nodeId = (int*) (buffer + sB.filenameSize);				//update inodeTable
        	    	// Find the right place for the inode
            		while(numOfiNode < *nodeId)
	            	{
        	        	*(inodeTable + offset) = false;                                 //empty
                		numOfiNode++;
                		offset += size;
	            	}
        	    	*(inodeTable + offset) = true;
            		memcpy(inodeTable+offset+sizeof(bool), buffer, size-sizeof(bool));
	            	numOfiNode++;
        	    	offset += size;

        //     CALL(lseek(fd,sB.filenameSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
        //     sum = 0;
        //     while(sum < sizeof(MDS))
        //     {
        //         CALL(read(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
        //         sum += n;
        //     }
        //     CALL(lseek(fd,sizeof(MDS),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
        //     sum = 0;
        //     while(sum < sizeof(unsigned int)*sB.maxDatablockNum)
        //     {
        //         CALL(read(fd,datablocks+sum,sizeof(unsigned int)*sB.maxDatablockNum),-1,"Error reading from cfs file: ",2,n);
        //         sum += n;
        //     }
        //     // Find the right place for the inode
        //     while(numOfiNode < metadata.nodeid)
        //     {
        //         *(inodeTable + offset) = false;                                 //empty
        //         *(inodeTable + offset + sB.filenameSize) = j;                   //nodeid
        //         offset += size;
        //         numOfiNode++;
        //     }
        //     *(inodeTable + offset) = true;
        //     offset += sizeof(bool);
        //     numOfiNode++;
        //     strcpy(inodeTable+offset,curr_name);
        //     offset += sB.filenameSize;
        //     memcpy(inodeTable + offset, &metadata, sizeof(MDS));
        //     offset += sizeof(MDS);
        //     memcpy(inodeTable + offset, &datablocks, sizeof(unsigned int)*sB.maxDatablockNum);
		}

		int	difference = sB.iTableBlocksNum*sizeof(unsigned int);
		int	overflow_block = sB.nextSuperBlock, overflow_prev, overflow_next;
		int	remainingblockSize = sB.blockSize - sizeof(int);
		while(difference > 0)										//find end of inodeTable
		{
			CALL(lseek(fd,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			overflow_prev = overflow_block;
//			sum = 0;
//			while(sum < sizeof(int))								//read overflow blocknum
//			{
//				plus = sum / sizeof(int);
//				CALL(read(fd,(&overflow_block)+plus,sizeof(int)),-1,"Error reading from cfs file: ",2,n);
//				sum += n;
//			}
			ptr_values = 0;
			SAFE_READ(fd,&overflow_block,ptr_values,sizeof(int),sizeof(int),sum,n,plus);

			if((difference > remainingblockSize) || (difference == remainingblockSize && sB.ListSize > 0))//if there is another overflow block
				difference -= remainingblockSize;
/*			else if(difference == sB.blockSize && sB.ListSize == 0)		//if inodeTable ends at the end of the block and there is no List yet
			{
////////////////////////CREATE NEW OVERFLOW_BLOCK FOR LIST ~IN PUSH//////////////////
				int	blocknum = (int) getEmptyBlock();			//get new block
				int	new_overflow = -1;

				CALL(lseek(fd,(-1)*sizeof(int),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
				sum = 0;
				while(sum < sizeof(int))					//write new block as old's overflow
				{
					CALL(write(fd,(&blocknum)+sum,sizeof(int)),-1,"Error writing in cfs file: ",3,n);
					sum += n;
				}

				CALL(lseek(fd,blocknum*blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);  //go to new block
				sum = 0;
				while(sum < sizeof(int))					//set new block's overflow as -1
				{
					CALL(write(fd,(&new_overflow)+sum,sizeof(int)),-1,"Error writing in cfs file: ",3,n);
					sum += n;
				}
				overflow_block = blocknum;
				difference = 1;
				break;
////////////////////////CREATE NEW OVERFLOW_BLOCK FOR LIST ~IN PUSH//////////////////
			}
*/
			else
			{
				overflow_next = overflow_block;
				overflow_block = overflow_prev;					//block where the inodeTable ends
				break;
			}
		}

        	unsigned int	blockNum;
		int		freeSpaces = remainingblockSize - difference;
        	for(int i=0; i<sB.ListSize; i++)
        	{
			//if there is enough space in current block to read
			if(freeSpaces > 0)
			{
				listOffset = overflow_block*sB.blockSize + (sB.blockSize - freeSpaces);
        			CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			}
			else
			{
				//if current block already has an overflow
				if(overflow_next != -1)
				{
					overflow_block = overflow_next;
					listOffset = overflow_block*sB.blockSize;
	            			CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
//					sum = 0;
//					while(sum < sizeof(int))
//					{
//						plus = sum / sizeof(int);
//						CALL(write(fd,(&overflow_next)+plus,sizeof(int)),-1,"Error writing in cfs file: ",3,n);
//						sum += n;
//					}
					ptr_values = 0;
					SAFE_WRITE(fd,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
				}
				//if an empty block is needed
				else
				{
					overflow_block = getEmptyBlock();
					listOffset = overflow_block*sB.blockSize;
		            		CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

		            		overflow_next = -1;
//            				sum = 0;
//            				while(sum < sizeof(int))
//            				{
//						plus = sum / sizeof(int);
//                				CALL(write(fd,(&overflow_next)+plus,sizeof(int)),-1,"Error writing from cfs file: ",2,n);
//	                			sum += n;
//        	    			}
					ptr_values = 0;
					SAFE_WRITE(fd,&overflow_next,ptr_values,sizeof(int),sizeof(int),sum,n,plus);
				}

				freeSpaces = remainingblockSize;
			}
			//read a list node (fileptr is on the right spot)
//			sum = 0;
// 	      		while(sum < sizeof(unsigned int))
//            		{
//				plus = sum / sizeof(unsigned int);
//                		CALL(read(fd,(&blockNum)+plus,sizeof(unsigned int)),-1,"Error reading from cfs file: ",2,n);
//                		sum += n;
//            		}
			ptr_values = 0;
			SAFE_READ(fd,&blockNum,ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);

            		addNode(holes, blockNum);

			freeSpaces -= sizeof(unsigned int);
        	}
	}

	return fd;
}

bool cfs_touch(int fd,char *filename,touch_mode mode)
{
	int	ignore = 0, ptr_values, sum, n, plus, move;
	bool	touched;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	if(mode == CRE)
	{
		int		i;
        	unsigned int	blocknum, parent_blocknum;
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
//		sum = 0;
//		while(sum < sizeof(MDS))
//		{
//			plus = sum / sizeof(MDS);
//			CALL(read(fd,(&parent_mds)+plus,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_READ(fd,&parent_mds,ptr_values,sizeof(MDS),sizeof(MDS),sum,n,plus);

		parent_data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));
//		sum = 0;
//		while(sum < (sB.maxDatablockNum*sizeof(unsigned int)))
//		{
//			plus = sum / sizeof(unsigned int);
//			CALL(read(fd,parent_data.datablocks+plus,sB.maxDatablockNum*sizeof(unsigned int)),-1,"Error reading from cfs file: ",2,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_READ(fd,parent_data.datablocks,ptr_values,sizeof(unsigned int),sB.maxDatablockNum*sizeof(unsigned int),sum,n,plus);

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
//		sum = 0;
//		while(sum < (strlen(filename)+1))								//new file's name
//		{
//			CALL(write(fd,filename+sum,strlen(filename)+1),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_WRITE(fd,filename,ptr_values,sizeof(char),strlen(filename)+1,sum,n,plus);

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
//		sum = 0;
//		while(sum < sizeof(MDS))								//new file's metadata
//		{
//			CALL(write(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_WRITE(fd,&metadata,ptr_values,sizeof(MDS),sizeof(MDS),sum,n,plus);

		data.datablocks = (unsigned int*)malloc(sB.maxDatablockNum*sizeof(unsigned int));
		for(int j=0; j<sB.maxDatablockNum; j++)							//new file's data
		{
			data.datablocks[j] = 0;
//			sum = 0;
//			while(sum < sizeof(unsigned int))
//			{
//				CALL(write(fd,(&(data.datablocks[j]))+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
//				sum += n;
//			}
			ptr_values = 0;
			SAFE_WRITE(fd,&(data.datablocks[j]),ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);
		}

		parent_data.datablocks[i] = blocknum;
		move = ((sB.blockSize)*(parent_blocknum)) + sB.filenameSize + sizeof(MDS) + (i*sizeof(unsigned int));
		CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
//		sum = 0;
//		while(sum < sizeof(unsigned int))								//update parent's data
//		{
//			CALL(write(fd,(&(parent_data.datablocks[i]))+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_WRITE(fd,&(parent_data.datablocks[i]),ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);

		free(parent_name);
		free(parent_data.datablocks);
		free(data.datablocks);
		touched = true;
	}
	else
	{
	        unsigned int	blocknum;
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
//			while(sum < sizeof(MDS))
//			{
//				CALL(read(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error reading from cfs file: ",2,n);
//				sum += n;
//			}
			ptr_values = 0;
			SAFE_READ(fd,&metadata,ptr_values,sizeof(MDS),sizeof(MDS),sum,n,plus);

			curr_time = time(NULL);
			if(mode == ACC)
				metadata.access_time = curr_time;
			else if(mode == MOD)
				metadata.modification_time = curr_time;

//			sum = 0;
//			while(sum < sizeof(MDS))
//			{
//				CALL(write(fd,(&metadata)+sum,sizeof(MDS)),-1,"Error writing in cfs file: ",3,n);
//				sum += n;
//			}
			ptr_values = 0;
			SAFE_WRITE(fd,&metadata,ptr_values,sizeof(MDS),sizeof(MDS),sum,n,plus);

			touched = true;
		}
	}

	return touched;
}

int cfs_create(char *filename,int bSize,int nameSize,int maxFSize,int maxDirFileNum)
{
	int		fd = 0, ignore = 0, ptr_values, sum, n, plus;
	time_t		current_time;
	MDS		metadata;
	Datastream	data;

	CALL(creat(filename, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,fd);

	sB.blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	if(bSize != -1 && bSize < BLOCK_SIZE)
		return -1;
	sB.filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	sB.maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	sB.maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	sB.maxDatablockNum = (sB.maxFileSize) / (sB.blockSize);
	// sB.metadataSize = sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
	sB.metadataBlocksNum = (sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) / sB.blockSize;
	if(((sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) % sB.blockSize) > 0)
        	sB.metadataBlocksNum++;
        
	sB.blockCounter = 0;										//assume block-counting is zero-based
	sB.nodeidCounter = 1;										//only root in file
	sB.ListSize = 0;
	sB.iTableBlocksNum = sB.nodeidCounter*sB.metadataBlocksNum;
	sB.iTableCounter = sB.nodeidCounter;

	sB.nextSuperBlock = (int)getEmptyBlock();
	CALL(lseek(fd,sB.nextSuperBlock*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	int	overflow_block = -1;
//	while(sum < sizeof(int))
//	{
//		CALL(write(fd,(&overflow_block)+sum,sizeof(int)),-1,"Error writing in cfs file: ",3,n);	//overflow blocknum
//		sum += n;
//	}
	ptr_values = 0;
	SAFE_WRITE(fd,&overflow_block,ptr_values,sizeof(int),sizeof(int),sum,n,plus);

///////////////CREATE ENTITY ///////////////////////
	unsigned int	blocknums[sB.metadataBlocksNum];
	for(int i=0; i<sB.metadataBlocksNum; i++)
	{
		blocknums[i] = getEmptyBlock();								//root's metadata blocknums
		if(i == 0)
			sB.root = blocknums[0];
//		sum = 0;
//		while(sum < sizeof(unsigned int))
//		{
//			CALL(write(fd,(&blocknums[i])+sum,sizeof(unsigned int)),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		ptr_values = 0;
		SAFE_WRITE(fd,&blocknums[i],ptr_values,sizeof(unsigned int),sizeof(unsigned int),sum,n,plus);
	}
	
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
//	sum = 0;
//	while(sum < sizeof(superBlock))									//write superBlock
//	{
//		CALL(write(fd,(&sB)+sum,sizeof(superBlock)),-1,"Error writing in cfs file: ",3,n);
//		sum += n;
//	}
	ptr_values = 0;
	SAFE_WRITE(fd,&sB,ptr_values,sizeof(superBlock),sizeof(superBlock),sum,n,plus);

	int	current = 0;
//	int	size = sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
	int	size_to_write, remainingSize, remainingblockSize, writtenSize = 0;

	CALL(lseek(fd,blocknums[current]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	CALL(write(fd,"/",2),-1,"Error writing in cfs file: ",3,ignore);				//write root's name
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);	//leave filenameSize space free

	remainingSize = sizeof(MDS);
	remainingblockSize = sB.blockSize - sB.filenameSize;
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
//		sum = 0;
//		while(sum < size_to_write)
//		{
//			CALL(write(fd,(&metadata)+writtenSize+sum,size_to_write),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		SAFE_WRITE(fd,&metadata,writtenSize,sizeof(MDS),size_to_write,sum,n,plus);

		remainingSize -= size_to_write;
		remainingblockSize -= size_to_write;
		writtenSize += size_to_write;
		if(remainingSize > 0 || (remainingSize == 0 && remainingblockSize == 0))
		{
			current++;
			CALL(lseek(fd,blocknums[current]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			remainingblockSize = sB.blockSize;
			size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;
		}
	}

	data.datablocks = (unsigned int*)malloc((sB.maxDatablockNum)*sizeof(unsigned int));			//root's data
	for(int i=0; i<(sB.maxDatablockNum); i++)
		data.datablocks[i] = 0;

	remainingSize = sB.maxDatablockNum*sizeof(unsigned int);
	size_to_write = (remainingSize > remainingblockSize) ? remainingblockSize : remainingSize;
	writtenSize = 0;

	while(remainingSize > 0)									//write root's data
	{
//		sum = 0;
//		while(sum < size_to_write)
//		{
//			CALL(write(fd,(data.datablocks)+(writtenSize/sizeof(unsigned int))+sum,size_to_write),-1,"Error writing in cfs file: ",3,n);
//			sum += n;
//		}
		SAFE_WRITE(fd,data.datablocks,writtenSize,sizeof(unsigned int),size_to_write,sum,n,plus);

		remainingSize -= size_to_write;
		remainingblockSize -= size_to_write;
		writtenSize += size_to_write;
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

bool cfs_close(int fileDesc,bool open_cfs) {
	int ignore = 0;
	if(fileDesc != -1 && open_cfs == true)
	{
        	update_superBlock(fileDesc);
        	free(inodeTable);

        	CALL(close(fileDesc),-1,"Error closing file for cfs: ",4,ignore);
        	return true;
    	}

	return false;
}
