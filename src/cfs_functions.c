/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

superBlock sB;

void update_superBlock(int fileDesc)
{
	int	ignore = 0, n, sum = 0;

	CALL(lseek(fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	while(sum < sizeof(superBlock))
	{
		CALL(write(fileDesc,(&sB)+sum,sizeof(superBlock)),-1,"Error writing in cfs file: ",3,n);
		sum += n;
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
