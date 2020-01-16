/* FILE: cfs_functions.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_functions.h"

Location getLocation(int fileDesc,int move,int mode)
{													//mode: SEEK_SET | SEEK_CUR | SEEK_END
	int		bytes;
	Location	loc;

	CALL(lseek(fileDesc,move,mode),-1,"Error moving ptr in cfs file: ",5,bytes);
	loc.blocknum = (int) (bytes / sB.blockSize) + 1;						//ignore superBlock (0 block)
	loc.offset = bytes % sB.blockSize;

	return loc;
}

Location traverse_cfs(int fileDesc,char *filename,unsigned int start_bl,unsigned int start_off)
{
	int		sum = 0, n, ignore = 0;
	char		*curr_name = (char*)malloc(sB.filenameSize*sizeof(char));

	CALL(lseek(fileDesc,((sB.blockSize)*start_bl)+start_off,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	while(sum < sB.filenameSize)
	{
		CALL(read(fileDesc,curr_name+sum,sB.filenameSize),-1,"Error reading from cfs file: ",3,n);
		sum += n;
	}
	if(!strcmp(curr_name,filename))
	{
		free(curr_name);
		return getLocation(fileDesc,0,SEEK_CUR);
	}
	else
	{
		unsigned int	curr_block, curr_offset;
		Datastream	data;
		Location	loc;

		data.datablocks = (Location*)malloc((sB.maxDatablockNum)*sizeof(Location));

		CALL(lseek(fileDesc,sizeof(MDS),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
		for(int i = 0; i<sB.maxDatablockNum; i++)
		{
			curr_block = (data.datablocks[i]).blocknum;
			curr_offset = (data.datablocks[i]).offset;
			if(curr_block == 0)
			{
				loc.blocknum = 0;
				loc.offset = 0;

				break;
			}
			else
			{
				loc = traverse_cfs(fileDesc,filename,curr_block,curr_offset);
				if(loc.blocknum)
					break;
			}
		}

		free(curr_name);
		return loc;
	}
}
