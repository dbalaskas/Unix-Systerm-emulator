/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

superBlock sB;
char *inodeTable;
List *holes;

void update_superBlock(int fileDesc)
{
	int             ignore = 0, n, sum = 0, iblock[sB.iTableBlocksNum], tableOffset = 0;
    unsigned int    new_blockNum,next_block;

	CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	while(sum < sizeof(superBlock))
	{
		CALL(write(fileDesc,(&sB)+sum,sizeof(superBlock)),-1,"Error writing in cfs file: ",3,n);
		sum += n;
	}
    
    //CALL(lseek(fileDesc,sizeof(superBlock)+i*sB.metadataBlocksNum*sizeof(int),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
    sum = 0;
    while(sum < sB.iTableBlocksNum*sizeof(int))
    {
        CALL(read(fileDesc,iblock+sum,sB.iTableBlocksNum*sizeof(int)),-1,"Error reading from cfs file: ",2,n);
        sum += n;
    }
    int size = sizeof(bool) + sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int);
    int size_to_write;
    for(int j=0; j<sB.iTableCounter; j++) {
        if (*(inodeTable+j*size) == true) {
            
            int remainingSize = size-sizeof(bool);
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
            int writtenSize = 0;
            for(int i=0; i<sB.metadataBlocksNum; i++) {
                if (tableOffset < sB.iTableBlocksNum) {
                    CALL(lseek(fileDesc,iblock[tableOffset]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
                } else {
                    new_blockNum = getEmptyBlock();
                    CALL(lseek(fileDesc,new_blockNum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
                }
                tableOffset++;

                size_to_write = (remainingSize > sB.blockSize) ? sB.blockSize : remainingSize;
                sum = 0;
                while(sum < size_to_write)
                {
                    CALL(write(fileDesc,inodeTable+j*size+sizeof(bool)+sum+writtenSize,size_to_write),-1,"Error writing from cfs file: ",2,n);
                    sum += n;
                }
                writtenSize += size_to_write;
                remainingSize -= size_to_write;

                if (tableOffset == sB.iTableBlocksNum) {
                    if (sizeof(superBlock) + (sB.iTableBlocksNum+1)*sizeof(int) < sB.blockSize) {
                        CALL(lseek(fileDesc,sizeof(superBlock) + sB.iTableBlocksNum*sizeof(int),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
                    } else {
                        sB.nextSuperBlock = getEmptyBlock();
                        CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

                        next_block = -1;
                        sum = 0;
                        while(sum < sizeof(int))
                        {
                            CALL(write(fileDesc,&(sB.nextSuperBlock),sizeof(int)),-1,"Error writing from cfs file: ",2,n);
                            sum += n;
                        }

                        // Initialize to -1 the new pointer of the superBlock
                        CALL(lseek(fileDesc,sB.nextSuperBlock*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

                        next_block = -1;
                        sum = 0;
                        while(sum < sizeof(int))
                        {
                            CALL(write(fileDesc,&next_block,sizeof(int)),-1,"Error writing from cfs file: ",2,n);
                            sum += n;
                        }
                    }

                    sum = 0;
                    while(sum < sizeof(int))
                    {
                        CALL(write(fileDesc,&new_blockNum,sizeof(int)),-1,"Error writing from cfs file: ",2,n);
                        sum += n;
                    }
                    sB.iTableBlocksNum++;
                }
            }

        }
    }

    unsigned int hole;
    CALL(lseek(fileDesc,sizeof(superBlock) + sB.iTableBlocksNum*sizeof(int),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
    for (int i=0; i<sB.ListSize;i++) {
        hole = pop(holes);
        if (sizeof(superBlock) + sB.iTableBlocksNum*sizeof(int)+i*sizeof(int) >= sB.blockSize) {
            sB.nextSuperBlock = getEmptyBlock();
            CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

            next_block = -1;
            sum = 0;
            while(sum < sizeof(int))
            {
                CALL(write(fileDesc,&(sB.nextSuperBlock),sizeof(int)),-1,"Error writing from cfs file: ",2,n);
                sum += n;
            }

            CALL(lseek(fileDesc,sB.nextSuperBlock*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

            next_block = -1;
            sum = 0;
            while(sum < sizeof(int))
            {
                CALL(write(fileDesc,&next_block,sizeof(int)),-1,"Error writing from cfs file: ",2,n);
                sum += n;
            }
        }

        sum = 0;
        while(sum < sizeof(int))
        {
            CALL(write(fileDesc,&hole,sizeof(int)),-1,"Error writing from cfs file: ",2,n);
            sum += n;
        }
    }
}

unsigned int getLocation(int fileDesc,int move,int mode)
{													//mode: SEEK_SET | SEEK_CUR | SEEK_END
	int		bytes, blocknum;

	CALL(lseek(fileDesc,move,mode),-1,"Error moving ptr in cfs file: ",5,bytes);
	blocknum = (int) (bytes / sB.blockSize);

	return blocknum;
}

unsigned int traverse_cfs(int fileDesc,char *filename,unsigned int start_bl)
{
	int		sum = 0, n, ignore = 0;
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
}

unsigned int getEmptyBlock()
{
    unsigned int new_blockNum = pop(holes);
    if(new_blockNum > 0) {
        sB.ListSize--;
        return new_blockNum;
    } else {
        sB.blockCounter++;
        return sB.blockCounter+1;
    }
}