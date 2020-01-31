/* FILE: cfs_commands.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include <dirent.h>
#include "../include/cfs_commands.h"

int cfs_workwith(cfs_info *info)
{
	int	ignore = 0;
	int	offset = 0, listOffset;
	int	overflow_block, read_iblocks, metadata_blocknum;
	int	current_iblock[((info->sB).iTableBlocksNum)];
	MDS	*metadata;

	// CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);
	if((info->fileDesc = open(info->fileName, O_RDWR)) == -1) {
		perror("Error opening file for cfs: ");
		return -1;
	}
	// We assume that we are in root at first, update global variable of our location
	info->cfs_current_nodeid = 0;

	// Go to the first block, read superBlock
	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_READ(info->fileDesc,&(info->sB),0,sizeof(superBlock),sizeof(superBlock));

	// (first) Block with the inodeTable and the holeList
	overflow_block = ((info->sB).nextSuperBlock);
	read_iblocks = 0;

	// Size of an inodeTable element
	info->inodeSize = sizeof(bool) + (info->sB).filenameSize + sizeof(MDS) + ((info->sB).maxFileDatablockNum)*sizeof(int);
	// Buffer to store info of an inodeTable element
	char	buffer[(info->inodeSize)-sizeof(bool)];
        int	size_to_read, dataSize, readSize;
	int	remainingDatablocks;
        int	*nodeId;
        int	numOfiNode = 0;
        info->inodeTable = (char*)malloc(((info->sB).iTableCounter)*(info->inodeSize));
        info->holes = NULL;

	// For every element of the inodeTable
	for(int j=0; j<((info->sB).nodeidCounter); j++)
	{
		// Start from the first block with the element's metadata
		metadata_blocknum = 0;

		// Will go to (the appropriate) block used by the inodeTable
		read_iblocks += sizeof(int);
		// Go to (the appropriate) block with the inodeTable
		CALL(lseek(info->fileDesc,(overflow_block*((info->sB).blockSize))+read_iblocks,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		// current_iblock[metadata] = appropriate block with metadata
		SAFE_READ(info->fileDesc,current_iblock,metadata_blocknum*sizeof(int),sizeof(int),sizeof(int));
		// Go to (the appropriate) block with metadata
        	CALL(lseek(info->fileDesc,current_iblock[metadata_blocknum]*((info->sB).blockSize),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		// Will read filename and MDS struct for now (stored easily in only one block)
		size_to_read = ((info->sB).filenameSize) + sizeof(MDS);
		SAFE_READ(info->fileDesc,buffer,metadata_blocknum*((info->sB).blockSize),sizeof(char),size_to_read);
		readSize = size_to_read;

		// Get datablocksCounter (number of datablocks) from metadata
		metadata = (MDS*) (buffer + ((info->sB).filenameSize));
		dataSize = (metadata->datablocksCounter)*sizeof(int);
		// Rest of datablocks in inodeTable that will be empty for now
		remainingDatablocks = ((info->sB).maxFileDatablockNum) - metadata->datablocksCounter;
		// Read all datablocks with contents (from the appropriate blocks)
		size_to_read = (dataSize > (((info->sB).blockSize))-readSize) ? (((info->sB).blockSize)-readSize) : (dataSize);
		// size_to_read must be a multiple of sizeof(int)
		if(size_to_read % sizeof(int))
			size_to_read -= size_to_read % sizeof(int);

		while(dataSize > 0)
		{
			SAFE_READ(info->fileDesc,buffer,(metadata_blocknum*((info->sB).blockSize))+readSize,sizeof(char),size_to_read);
			dataSize -= size_to_read;

			// If there are more metadata in another block
			if(dataSize > 0)
			{
				// Go to the next block of metadata
				metadata_blocknum++;
				// Go to the next block used by the inodeTable
				read_iblocks += sizeof(int);

				// If inodeTable has more elements, it is continued to another block (overflow)
				if(read_iblocks == ((info->sB).blockSize))
				{
					// Get overflow block
					CALL(lseek(info->fileDesc,overflow_block*((info->sB).blockSize),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(info->fileDesc,&overflow_block,0,sizeof(int),sizeof(int));
					read_iblocks = sizeof(int);
				}

				// Go to (the appropriate) block with the inodeTable
				CALL(lseek(info->fileDesc,(overflow_block*((info->sB).blockSize))+read_iblocks,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				// current_iblock[metadata] = appropriate block with metadata
				SAFE_READ(info->fileDesc,current_iblock,metadata_blocknum*sizeof(int),sizeof(int),sizeof(int));
				// Go to (the appropriate) block with metadata
		        	CALL(lseek(info->fileDesc,current_iblock[metadata_blocknum]*((info->sB).blockSize),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				readSize = 0;
				size_to_read =(dataSize > (((info->sB).blockSize)-readSize)) ? (((info->sB).blockSize)-readSize) : (dataSize);
				// size_to_read must be a multiple of sizeof(int)
				if(size_to_read % sizeof(int))
					size_to_read -= size_to_read % sizeof(int);
			}
		}

	        nodeId = (int*) (buffer + ((info->sB).filenameSize));				//update inodeTable
        	// Find the right place for the inode ~based on nodeid~ (holes in the inodeTable have to be maintained)
            	while(numOfiNode < *nodeId)
	        {
        	       	*(info->inodeTable + offset) = false;                                 //empty
                	numOfiNode++;
                	offset += (info->inodeSize);
	        }

		*(bool*)(info->inodeTable + offset) = true;
		offset += sizeof(bool);
		// Copy inodeTable from cfs file to table structure
		memcpy((info->inodeTable)+offset, buffer, (info->inodeSize)-sizeof(bool)-(remainingDatablocks*sizeof(int)));
		int	number = -1;
		offset += (info->inodeSize) - sizeof(bool) - (remainingDatablocks*sizeof(int));
		for(int k=0; k<remainingDatablocks; k++)
		{
			memcpy((info->inodeTable)+offset, &number , sizeof(int));
			offset += sizeof(int);
		}

		// Go to inodeTable's next element
	        numOfiNode++;
	}

	int	difference = ((info->sB).iTableBlocksNum)*sizeof(int);
	int	overflow_prev, overflow_next;
	int	remainingblockSize = ((info->sB).blockSize) - sizeof(int);
	// Start from first block with the inodeTable
	overflow_block = ((info->sB).nextSuperBlock);
	// Find the end of the inodeTable
	while(difference > 0)
	{
		CALL(lseek(info->fileDesc,overflow_block*((info->sB).blockSize),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		overflow_prev = overflow_block;
		SAFE_READ(info->fileDesc,&overflow_block,0,sizeof(int),sizeof(int));

		// If there is another overflow block
		if((difference > remainingblockSize) || (difference == remainingblockSize && ((info->sB).ListSize) > 0))
			difference -= remainingblockSize;
		// InodeTable ends in current block and there is space left in the same block
		else
		{
			overflow_next = overflow_block;
			overflow_block = overflow_prev;					//block where the inodeTable ends
			break;
		}
	}

	int	blockNum;
	// Number of bytes left in current block, after the inodeTable
	int	freeSpaces = remainingblockSize - difference;
	// Read list of holes
	for(int i=0; i<((info->sB).ListSize); i++)
        {
		// If there is enough space in current block to read
		if(freeSpaces > 0)
		{
			listOffset = overflow_block*((info->sB).blockSize) + (((info->sB).blockSize) - freeSpaces);
        		CALL(lseek(info->fileDesc,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		}
		else
		{
			// If current block already has an overflow
			if(overflow_next != -1)
			{
				overflow_block = overflow_next;
				listOffset = overflow_block*((info->sB).blockSize);
	       			CALL(lseek(info->fileDesc,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
			}
			// If an empty block is needed
			else
			{
				overflow_block = getEmptyBlock(info);
				listOffset = overflow_block*((info->sB).blockSize);
	            		CALL(lseek(info->fileDesc,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		            		overflow_next = -1;
				SAFE_WRITE(info->fileDesc,&overflow_next,0,sizeof(int),sizeof(int));
			}
				freeSpaces = remainingblockSize;
		}
		// Read a list node (fileptr is on the right spot)
		SAFE_READ(info->fileDesc,&blockNum,0,sizeof(int),sizeof(int));
		// Push it in List structure
       		addNode(&(info->holes), blockNum);
		// Space in current block is reduced by one list node
		freeSpaces -= sizeof(int);
	}

	return info->fileDesc;
}

int cfs_mkdir(cfs_info *info, char *dirname)
{
	int		ignore = 0;
	int		new_nodeid;
	int		parent_nodeid, i, offset;
	char	parent_path[strlen(dirname)+1];
	char		new_name[(info->sB).filenameSize+1];
	bool		busy;
	time_t		curr_time;
	MDS		*parent_mds, *metadata;
	Datastream	parent_data, data;

	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		
	// Initialize new_name
	for(int i=0; i<=(info->sB).filenameSize; i++)
		new_name[i] = '\0';

	// Get rid of possible extra '/'
	cleanSlashes(&dirname);
	// Get parent's nodeid and new directory's name
	parent_nodeid = get_parent(info,dirname,new_name);
	if(parent_nodeid == -1)
		return -1;
	// Go to parent's metadata
	parent_mds = (MDS*) (info->inodeTable + parent_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	parent_mds->access_time = time(NULL);
	// Check if parent is a directory
	if(parent_mds->type != Directory)
	{
		printf("mkdir: Input error, %s is not a directory.\n",info->inodeTable+parent_nodeid*(info->inodeSize)+sizeof(bool));
		return -1;
	}

	// Check if new directory exists
	if(traverse_cfs(info,new_name) != -1)
	{
		printf("mkdir: Input error, directory %s already exists.\n",dirname);
		return -1;
	}

	// Go to parent's data
	parent_data.datablocks = (int*)(info->inodeTable + parent_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));

	int	dataCounter, move, numofEntities;
	if (strlen(dirname)-strlen(new_name) > 0) {
		strncpy(parent_path, dirname, strlen(dirname)-strlen(new_name));
		parent_path[strlen(dirname)-strlen(new_name)] = '\0';
	} else {
		strcpy(parent_path, "./");
	}
	numofEntities = getDirEntities(info,parent_path,NULL);

	// Update parent directory's data
	for(i=0; i<(info->sB).maxFileDatablockNum; i++)
	{
		// If current datablock is empty
		if(parent_data.datablocks[i] == -1)
		{
			parent_data.datablocks[i] = getEmptyBlock(info);
			// Go to datablock
			move = parent_data.datablocks[i]*(info->sB).blockSize + sizeof(int);
			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			dataCounter = 0;
			parent_mds->datablocksCounter++;
		}
		else
		{
			// Go to datablock
			move = parent_data.datablocks[i]*(info->sB).blockSize;
			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
		}

		// If parent directory has no datablocks available
		if(parent_mds->datablocksCounter == (info->sB).maxDirDatablockNum && numofEntities == (info->sB).maxEntitiesPerBlock*(info->sB).maxDirDatablockNum)
		{
			printf("mkdir: Not enough space in parent directory to create subdirectory %s.\n",dirname);
			return -1;
		}
	
		// If there is space for another entity
		if(dataCounter < (info->sB).maxEntitiesPerBlock)
		{
			// Find an empty space in inodeTable
			new_nodeid = getTableSpace(info);

			// Go after the last entity in the datablockrealloc
			CALL(lseek(info->fileDesc,dataCounter*((info->sB).filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(info->fileDesc,new_name,0,sizeof(char),(info->sB).filenameSize);
			SAFE_WRITE(info->fileDesc,&new_nodeid,0,sizeof(int),sizeof(int));
			// Increase datablock's counter
			dataCounter++;
			CALL(lseek(info->fileDesc,(parent_data.datablocks[i])*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			// If this is the first entity in datablock
			break;
		}
	}

	parent_mds->size += (info->sB).filenameSize + sizeof(int);

	(info->sB).nodeidCounter++;
	(info->sB).iTableCounter++;
	// Allocate space for another element in inodeTable
	info->inodeTable = (char*)realloc(info->inodeTable,(info->sB).iTableCounter*(info->inodeSize));
	offset = new_nodeid*(info->inodeSize);
	// Fill empty space with new file
	busy = true;
	*(bool*) (info->inodeTable + offset) = busy;

	offset += sizeof(bool);
	strcpy(info->inodeTable + offset,new_name);

	offset += (info->sB).filenameSize;
	metadata = (MDS*) (info->inodeTable + offset);
	metadata->nodeid = new_nodeid;
	metadata->size = 2*((info->sB).filenameSize + sizeof(int));
	metadata->type = Directory;
	metadata->parent_nodeid = parent_nodeid;
	curr_time = time(NULL);
	metadata->creation_time = curr_time;
	metadata->access_time = curr_time;
	metadata->modification_time = curr_time;
	metadata->datablocksCounter = 1;

	// Create directory's data (one block for now)
	offset += sizeof(MDS);
	data.datablocks = (int*) (info->inodeTable + offset);
	data.datablocks[0] = getEmptyBlock(info);
	CALL(lseek(info->fileDesc,data.datablocks[0]*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	// Write entities' counter for current datablock
	int	entityCounter = 2;
	SAFE_WRITE(info->fileDesc,&entityCounter,0,sizeof(int),sizeof(int));
	// Write entity (filename + nodeid) for relative path "."
	CALL(write(info->fileDesc,".",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(info->fileDesc,(info->sB).filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(metadata->nodeid),0,sizeof(int),sizeof(int));
	// Write entity for relative path ".."
	CALL(write(info->fileDesc,"..",3),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(info->fileDesc,(info->sB).filenameSize-3,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(metadata->parent_nodeid),0,sizeof(int),sizeof(int));
	// Directory's rest datablocks will be empty for now
	for(i=1; i<(info->sB).maxFileDatablockNum; i++)
		data.datablocks[i] = -1;

	parent_mds->modification_time = time(NULL);
	return new_nodeid;
}

int cfs_touch(cfs_info *info, char *filename,touch_mode mode)
{
	int	ignore = 0;

	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	if(mode == CRE)
	{
		int		parent_nodeid, new_nodeid;
		int		i, offset;
		char		new_name[(info->sB).filenameSize+1];
		char		parent_path[strlen(filename)+1];
		bool		busy;
		time_t		curr_time;
		MDS		*parent_mds, *metadata;
		Datastream	parent_data, data;

		// Initialize new_name
		for(int i=0; i<=(info->sB).filenameSize; i++)
			new_name[i] = '\0';

		// Get rid of possible extra '/'
		cleanSlashes(&filename);
		// Get parent's nodeid and new file's name
		parent_nodeid = get_parent(info,filename,new_name);
		if(parent_nodeid == -1)
			return -1;
	
		// Go to parent's metadata
		parent_mds = (MDS*) (info->inodeTable + parent_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		parent_mds->access_time = time(NULL);
		// Check if parent is a directory
		if(parent_mds->type != Directory)
		{
			printf("Input error, %s is not a directory.\n",info->inodeTable+parent_nodeid*(info->inodeSize)+sizeof(bool));
			return -1;
		}

		// Check if file exists
		if(traverse_cfs(info,new_name) != -1)
		{
			printf("Input error, file %s already exists.\n",filename);
			return -1;
		}

		// Go to parent's data
		parent_data.datablocks = (int*)(info->inodeTable + parent_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));

		int	dataCounter, move, numofEntities;
		if (strlen(filename)-strlen(new_name) > 0) {
			strncpy(parent_path, filename, strlen(filename)-strlen(new_name));
			parent_path[strlen(filename)-strlen(new_name)] = '\0';
		} else {
			strcpy(parent_path, "./");
		}
		numofEntities = getDirEntities(info,parent_path,NULL);

		// Update parent directory's data
		for(i=0; i<(info->sB).maxFileDatablockNum; i++)
		{
			// If current datablock is empty
			if(parent_data.datablocks[i] == -1)
			{
				parent_data.datablocks[i] = getEmptyBlock(info);
				// Go to datablock
				move = parent_data.datablocks[i]*(info->sB).blockSize + sizeof(int);
				CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				dataCounter = 0;
				parent_mds->datablocksCounter++;
			}
			else
			{
				// Go to datablock
				move = parent_data.datablocks[i]*(info->sB).blockSize;
				CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			}

			// If parent directory has no datablocks available
			if(parent_mds->datablocksCounter == (info->sB).maxDirDatablockNum && numofEntities == (info->sB).maxEntitiesPerBlock*(info->sB).maxDirDatablockNum)
			{
				printf("touch: Not enough space in parent directory to create file %s.\n",filename);
				return -1;
			}
	
			// If there is space for another entity
			if(dataCounter < (info->sB).maxEntitiesPerBlock)
			{
				// Find an empty space in inodeTable
				new_nodeid = getTableSpace(info);

				// Go after the last entity in the datablockrealloc
				CALL(lseek(info->fileDesc,dataCounter*((info->sB).filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(info->fileDesc,new_name,0,sizeof(char),(info->sB).filenameSize);
				SAFE_WRITE(info->fileDesc,&new_nodeid,0,sizeof(int),sizeof(int));
				// Increase datablock's counter
				dataCounter++;
				CALL(lseek(info->fileDesc,(parent_data.datablocks[i])*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
				break;
			}
		}

		parent_mds->size += (info->sB).filenameSize + sizeof(int);

		(info->sB).nodeidCounter++;
		(info->sB).iTableCounter++;
		// Allocate space for another element in inodeTable
		info->inodeTable = (char*)realloc(info->inodeTable,(info->sB).iTableCounter*(info->inodeSize));
		offset = new_nodeid*(info->inodeSize);
		// Fill empty space with new file
		busy = true;
		*(bool*) (info->inodeTable + offset) = busy;

		offset += sizeof(bool);
		strcpy(info->inodeTable + offset,new_name);

		offset += (info->sB).filenameSize;
		metadata = (MDS*) (info->inodeTable + offset);
		metadata->nodeid = new_nodeid;
		metadata->size = 0;
		metadata->type = File;
		metadata->parent_nodeid = parent_nodeid;
		curr_time = time(NULL);
		metadata->creation_time = curr_time;
		metadata->access_time = curr_time;
		metadata->modification_time = curr_time;
		metadata->datablocksCounter = 0;

		offset += sizeof(MDS);
		data.datablocks = (int*) (info->inodeTable + offset);
		for(i=0; i<(info->sB).maxFileDatablockNum; i++)
			data.datablocks[i] = -1;

		parent_mds->modification_time = time(NULL);
		return new_nodeid;
	}
	else
	{
		int	nodeid;
		time_t	curr_time;
		MDS	*metadata;

		// Find entity in cfs
		nodeid = traverse_cfs(info,filename);
		// If path was invalid
		if(nodeid == -1)
		{
			printf("touch: Path %s could not be found in cfs.\n",filename);
			return -1;
		}
		else
		{
			metadata = (MDS*) (info->inodeTable + nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			curr_time = time(NULL);
			if(mode == ACC)
				metadata->access_time = curr_time;
			else if(mode == MOD)
				metadata->modification_time = curr_time;

		}

		return nodeid;
	}
}

int cfs_ln(cfs_info *info, char* source_path,char* destination)
{
	int 	 ignore;

	char 	source_name[(info->sB).filenameSize];
	int 	 source_nodeid;
	MDS		*source_mds;

	int 	 directory_nodeid;
	MDS		*directory_mds;
	int		*directory_data;

	char	 destination_path[strlen(destination) + (info->sB).filenameSize + 2];
	char	 destination_name[(info->sB).filenameSize];
	int		 destination_nodeid;
	MDS		*destination_mds;

	// Get rid of possible extra '/'
	cleanSlashes(&source_path);
	cleanSlashes(&destination);
	// Check if source exists.
	get_parent(info, source_path, source_name);
	source_nodeid = traverse_cfs(info, source_path);
	if (source_nodeid == -1) {
		printf("ln: cannot stat '%s': No such file or directory\n",source_path);
		return -1;
	}
	// Check if source is file.
	source_mds = (MDS*) (info->inodeTable + source_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	source_mds->access_time = time(NULL);
	if (source_mds->type == Directory) {
		printf("ln: cannot create link to directory\n");
		return -1;
	}
	// Check if destination exists.
	directory_nodeid = get_parent(info, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("ln: cannot create directory '%s': No such file or directory\n", destination);
		return -1;
	}

	destination_nodeid = traverse_cfs(info,destination_name);
	strcpy(destination_path, destination);
	if (destination_nodeid != -1) {
		destination_mds = (MDS*) (info->inodeTable + destination_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		destination_mds->access_time = time(NULL);
		if (destination_mds->type == Directory) {
			strcpy(destination_name, source_name);
			directory_nodeid = destination_nodeid;
			strcat(destination_path, "/");
			strcat(destination_path, source_name);
		} else {
			printf("ln: cannot overwrite an entity at '%s' with link\n", destination_path);
			return -1;
		}
	}
	directory_mds = (MDS*) (info->inodeTable + directory_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	directory_mds->access_time = time(NULL);
	// Go to directory's data.
	directory_data = (int*)(info->inodeTable + directory_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));
	int	dataCounter, move, numofEntities = 0;

	// Update directory's data.
	for(int i=0; i<(info->sB).maxFileDatablockNum; i++)
	{
		// Check if current datablock is empty.
		if(directory_data[i] == -1)
		{
			directory_data[i] = getEmptyBlock(info);
			// Go to datablock
			move = directory_data[i]*(info->sB).blockSize + sizeof(int);
			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			dataCounter = 0;
			directory_mds->datablocksCounter++;
		}
		else
		{
			// Go to datablock
			move = directory_data[i]*(info->sB).blockSize;
			CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			numofEntities += dataCounter;
		}
		// If directory has no datablocks available
		if(directory_mds->datablocksCounter == (info->sB).maxDirDatablockNum && numofEntities == (info->sB).maxEntitiesPerBlock*(info->sB).maxDirDatablockNum)
		{
			printf("ln: not enough space in directory to create link '%s' to '%s'.\n",source_name, destination_path);
			return -1;
		}
		// If there is space for another entity
		if(dataCounter < (info->sB).maxEntitiesPerBlock)
		{
			source_mds->linkCounter++;

			// Go after the last entity in the datablock
			CALL(lseek(info->fileDesc,dataCounter*((info->sB).filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(info->fileDesc,destination_name,0,sizeof(char),(info->sB).filenameSize);
			SAFE_WRITE(info->fileDesc,&source_nodeid,0,sizeof(int),sizeof(int));
			// Increase datablock's counter
			dataCounter++;
			CALL(lseek(info->fileDesc,(directory_data[i])*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			break;
		}
	}

	directory_mds->size += (info->sB).filenameSize + sizeof(int);
	directory_mds->modification_time = time(NULL);
	return source_nodeid;
}

bool cfs_pwd(cfs_info *info) 
{
	// Takes the current nodeid from global variable cfs_current_nodeid,
	int 	 		 curr_dir_id = info->cfs_current_nodeid;
	MDS 			*curr_dir_MDS = (MDS *) (info->inodeTable + (info->cfs_current_nodeid)*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);

	// Traverse the nodes until the root. For each one save its id in a list path_nodeIds,
	List 			*path_nodeIds = NULL;
	while (curr_dir_id != 0) {
		addNode(&path_nodeIds, curr_dir_id);
		curr_dir_id = curr_dir_MDS->parent_nodeid;
		curr_dir_MDS = (MDS *) (info->inodeTable + curr_dir_id*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	}
	// Now the list path_nodeIds has the nodeids of each dir on the path to current dir, from start to end.
	// So we pop each one and print its name,
	char 	*fileName = NULL;
	printf("~");
	if (path_nodeIds == NULL) {
		// We are at the root
		printf("/");
	} else {
		// We are not at the root,
		while (path_nodeIds != NULL) {
			curr_dir_id = pop(&path_nodeIds);
			curr_dir_MDS = (MDS *) (info->inodeTable + (info->cfs_current_nodeid)*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			fileName = (info->inodeTable + curr_dir_id*(info->inodeSize) + sizeof(bool));;
			printf("/%s", fileName);
		}
	}
	return true;
}

bool cfs_cd(cfs_info *info, char *path) 
{
	/*
	  cfs_cd: Changes the cfs_current_nodeid.
	*/
	int 	 path_nodeId = -1;
	MDS 	*path_MDS = NULL;

	// Check if the path has value in it.
	if (path == NULL || strcmp(path, "/") == 0) {
		// If was not given a path, then find then set current nodeid as the root id (cd to root).
		info->cfs_current_nodeid = 0;
		return true;
	}
	// Get rid of possible extra '/'
	cleanSlashes(&path);
	// Find the nodeid of the path.
	path_nodeId = traverse_cfs(info, path);
	// Check if the path has valid value.
	if(path_nodeId == -1) {
		printf("cd: %s: No such file or directory.\n", path);
		return false;
	} else {
		path_MDS = (MDS *) (info->inodeTable + path_nodeId*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		path_MDS->access_time = time(NULL);
	}
	// Check if the last entity is directory.
	if (path_MDS->type != Directory) {
		printf("cd: %s: Not a directory.\n", path);
		return false;
	} else {
		info->cfs_current_nodeid = path_nodeId;
	}

	return true;
}

bool cfs_ls(cfs_info *info, bool *ls_modes, char *path) 
{
	int 	 		 ignore;
	int 	 		 path_nodeId = -1;
	List 			*path_content = NULL;
	string_List 	*path_content_names = NULL;
	MDS 			*path_MDS = NULL;
	int				*path_data;
	int				 current_nodeid;
	char 			*current_filename;
	MDS 			*current_MDS;
	char			*dirName = NULL;

	// Check if the path has value in it.
	if (path == NULL) {
		// If was not given a path, then set path nodeid as the root id (ls to root).
		path_nodeId = info->cfs_current_nodeid;
		path_MDS = (MDS *) (info->inodeTable + path_nodeId*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		path_MDS->access_time = time(NULL);
	} else {
		// Get rid of possible extra '/'
		cleanSlashes(&path);
		path_nodeId = traverse_cfs(info, path);

		// Check if the path has valid value.
		if(path_nodeId != -1) {
			path_MDS = (MDS *) (info->inodeTable + path_nodeId*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			path_MDS->access_time = time(NULL);
		} else {
			printf("ls: cannot access '%s': No such file or directory.\n",path);
			return false;
		}
		// Check if the returned id is a directory. If it is not print error.
		if (path_MDS->type != Directory) {
			printf("ls: target '%s' is not a directory.\n",path);
			return false;
		}
	}

	// Get the list with the nodeids of the path's content.
	path_data = (int *) (info->inodeTable + path_nodeId*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));
	int dataStreamIndex=0;
	int pairCounter;
	current_filename = (char*) malloc((info->sB).filenameSize);
	for (int i=0; i < path_MDS->datablocksCounter; i++) {
		// Ignore the cells of the table with -1 value.
		while (path_data[dataStreamIndex] == -1) {
			dataStreamIndex++;
		}
		CALL(lseek(info->fileDesc,path_data[dataStreamIndex]*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		SAFE_READ(info->fileDesc,&pairCounter,0,sizeof(int),sizeof(int));
		for (int j=0; j < pairCounter; j++) {
			CALL(lseek(info->fileDesc,path_data[dataStreamIndex]*(info->sB).blockSize + sizeof(int) + j*((info->sB).filenameSize+sizeof(int)),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,current_filename,0,sizeof(char),(info->sB).filenameSize);
			SAFE_READ(info->fileDesc,&current_nodeid,0,sizeof(int),sizeof(int));
			current_MDS = (MDS *) (info->inodeTable + current_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			current_MDS->access_time = time(NULL);
			if(strcmp(current_filename,".") && strcmp(current_filename,"..") && current_MDS->type == Directory)
				addNode(&path_content, current_nodeid);
			if ((ls_modes[LS_D] == true && current_MDS->type == Directory) || (ls_modes[LS_H] == true && current_MDS->type == File && current_MDS->linkCounter > 1) || (ls_modes[LS_D] == false && ls_modes[LS_H] == false))
				add_stringNode(&path_content_names, current_filename);
		}
		dataStreamIndex++;
	}
	free(current_filename);
	// Print them, with the right option.
	// If -r was given then at the first line prints the path of the directory.
	if (ls_modes[LS_R] == true) {
		if(path == NULL) {
			dirName = malloc(2);
			strcpy(dirName, "."); 
		} else {
			dirName = malloc(strlen(path)+1);
			strcpy(dirName, path);
		}
		printf("%s:\n", dirName);
	}
	// Prints the content of the directory.
	while (path_content_names != NULL) {
		if (ls_modes[LS_U] == true) {
			current_filename = pop_last_string(&path_content_names);
		} else {
			current_filename = pop_minimum_string(&path_content_names);
		}
		if ((ls_modes[LS_A] == false && current_filename[0] != '.') || ls_modes[LS_A] == true) {
			current_nodeid = traverse_cfs(info, current_filename);
			current_MDS = (MDS *) (info->inodeTable + current_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			current_MDS->access_time = time(NULL);
			if (ls_modes[LS_L] == true) {
				char *creationTime = ctime(&current_MDS->creation_time);
				char *accessTime = ctime(&current_MDS->access_time);
				char *modificationTime = ctime(&current_MDS->modification_time);
				creationTime[strlen(creationTime)-1] = '\0';
				accessTime[strlen(accessTime)-1] = '\0';
				modificationTime[strlen(modificationTime)-1] = '\0';
				if (current_MDS->type == Directory)
					printf("%s/\t: \t%s \t%s \t%s \t%dB \n", current_filename, creationTime, accessTime, modificationTime, current_MDS->size);
				else
					printf("%s\t: \t%s \t%s \t%s \t%dB \n", current_filename, creationTime, accessTime, modificationTime, current_MDS->size);
			} else {
				if (current_MDS->type == Directory)
					printf("%s/  ", current_filename);
				else
					printf("%s  ", current_filename);
			}
		}
		free(current_filename);
	}
	// If -r was given, when it print the content of the directory starts to call recursivly the ls for the subirectories with the same modes.
	if (ls_modes[LS_R] == true) {
		char *temp = (char*) malloc(strlen(dirName)+1);
		int tempSize = strlen(temp);
		int dirPathSize;
		while (path_content != NULL) {
			strcpy(temp, dirName);
			current_nodeid = pop_last_Node(&path_content);
			current_filename = info->inodeTable + current_nodeid*(info->inodeSize) + sizeof(bool);
			current_MDS = (MDS *) (info->inodeTable + current_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			current_MDS->access_time = time(NULL);
			dirPathSize = strlen(temp) + strlen(current_filename) + 2;
			if (tempSize < dirPathSize)
				temp = realloc(temp, dirPathSize);
			if(strcmp(temp,"/"))
				strcat(temp, "/");
			strcat(temp, current_filename);
			
			printf("\n");
			cfs_ls(info, ls_modes, temp);
		}
		free(temp);
	}
	free(dirName);
	return true;
}

bool cfs_cp(cfs_info *info, bool *cp_modes, string_List *sourceList, char *destination)
{
	time_t			 current_time;
	int 		 sourcesCount = getLength(sourceList);
	char		 line[100];
	char		*answer;
	string_List *directory_inodeList;

	char 		*source_path;
	char 		 source_name[(info->sB).filenameSize];
	int 		 source_nodeid;
	MDS 		*source_mds;

	int 		 directory_nodeid;

	char		 destination_path[strlen(destination) + (info->sB).filenameSize + 2];
	char 		 destination_name[(info->sB).filenameSize];
	int			 destination_nodeid;
	MDS 		*destination_mds;

	//------------------------------------------------------------------------------------
	// Get destination's directory inode
	directory_nodeid = get_parent(info, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("cp: cannot create directory '%s': No such file or directory\n", destination);
		return false;
	}
	//------------------------------------------------------------------------------------
	// Copy each source to destination.
	while (sourceList != NULL) {
		source_path = pop_last_string(&sourceList);
		// Get rid of possible extra '/'
		cleanSlashes(&source_path);
		// Get source inode.
		get_parent(info, source_path, source_name);
		source_nodeid = traverse_cfs(info, source_path);
		// Check if source_path has valid value.
		if(source_nodeid == -1) {
			printf("cp: cannot stat '%s': No such file or directory\n",source_path);
			free(source_path);
			continue;
		}
		source_mds = (MDS*) (info->inodeTable + source_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		source_mds->access_time = time(NULL);
		if (source_mds->type == Directory && cp_modes[CP_R] == false && cp_modes[CP_RR] == false) {
			printf("cp: -r not specified; omitting directory '%s'\n", source_path);
			free(source_path);
			continue;
		}
		//--------------------------------------------------------------------------------
		// Get rid of possible extra '/'
		cleanSlashes(&destination);
		// Get destination inode.
		destination_nodeid = traverse_cfs(info,destination_name);
		strcpy(destination_path, destination);
		if (destination_nodeid == -1) {
			if (sourcesCount != 1) {
				printf("cp: target '%s' is not a directory\n", destination);
				free(source_path);
			} else if (source_mds->type == Directory) {
				if (cp_modes[CP_RR] == true) {
					cfs_mkdir(info, destination_path);
				} else {	// (cp_modes[CP_R] == true)
					directory_inodeList = NULL;
					getDirEntities(info, source_path, &directory_inodeList);
					cfs_mkdir(info, destination_path);
					cfs_cp(info, cp_modes, directory_inodeList, destination_path);
				}
			} else {
				cfs_touch(info, destination_path, CRE);
				destination_nodeid = traverse_cfs(info, destination_path);
				replaceEntity(info, source_nodeid, destination_nodeid);
			}
		} else {	// destination_nodeid != -1
			// If the file exists check the its type.
			destination_mds = (MDS*) (info->inodeTable + destination_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			destination_mds->access_time = time(NULL);
			if (destination_mds->type == Directory) {
				strcat(destination_path, "/");
				strcat(destination_path, source_name);
				destination_nodeid = traverse_cfs(info,source_name);
				if (destination_nodeid == -1) {
					if (source_mds->type == Directory) {
						if (cp_modes[CP_RR] == true) {
							cfs_mkdir(info, destination_path);
						} else {	// (cp_modes[CP_R] == true)
							directory_inodeList = NULL;
							getDirEntities(info, source_path, &directory_inodeList);
							cfs_mkdir(info, destination_path);
							cfs_cp(info, cp_modes, directory_inodeList, destination_path);
						}
					} else {
						cfs_touch(info, destination_path, CRE);
						destination_nodeid = traverse_cfs(info, destination_path);
						replaceEntity(info, source_nodeid, destination_nodeid);
					}
				} else {	// destination_nodeid != -1.
					// If destination/source_name exists check if the type of source and destination are matching.
					destination_mds = (MDS*) (info->inodeTable + destination_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
					destination_mds->access_time = time(NULL);
					if (source_mds->type == Directory && destination_mds->type == Directory) {
						if (cp_modes[CP_I] == true) {
							printf("cp: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Copy denied.\n");
								free(source_path);
								continue;
							}
						}
						directory_inodeList = NULL;
						getDirEntities(info, source_path, &directory_inodeList);
						cfs_cp(info, cp_modes, directory_inodeList, destination_path);
					} else if (source_mds->type == File && destination_mds->type == File) {
						if (cp_modes[CP_I] == true) {
							printf("cp: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Copy denied.\n");
								free(source_path);
								continue;
							}
						}
						current_time = time(NULL);
						destination_mds->creation_time = current_time;
						destination_mds->access_time = current_time;
						destination_mds->modification_time = current_time;
						replaceEntity(info, source_nodeid, destination_nodeid);
						destination_mds->linkCounter = 1;
					} else if (source_mds->type != Directory && destination_mds->type == Directory) {
						printf("cp: cannot overwrite directory '%s' with non-directory '%s'\n", destination_path, source_path);
					} else if (source_mds->type == Directory && destination_mds->type != Directory) {
						printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", destination_path, source_path);
					}
				}
			} else {	// destination_mds->type == File
				// If destination is not a directory, first check the sourcesCount and after the source type.
				if (sourcesCount != 1) {
					printf("cp: target '%s' is not a directory\n", destination);
				} else if (source_mds->type == Directory) {
					printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", destination, source_path);
				} else if (source_mds->type == File) {
					if (cp_modes[CP_I] == true) {
						printf("cp: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
						fgets(line, 100, stdin);
						if (line != NULL){
							answer = strtok(line, " \n\t");
						}
						if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
							printf("Copy denied.\n");
							free(source_path);
							continue;
						}
					}
					current_time = time(NULL);
					destination_mds->creation_time = current_time;
					destination_mds->access_time = current_time;
					destination_mds->modification_time = current_time;
					replaceEntity(info, source_nodeid, destination_nodeid);
					destination_mds->linkCounter = 1;
				}
			}
		}
		free(source_path);
	}
	return true;
}

bool cfs_mv(cfs_info *info, bool *mv_modes, string_List *sourceList, char *destination)
{
	time_t		 current_time;
	int 		 sourcesCount = getLength(sourceList);
	char		 line[100];
	char		*answer;
	string_List *directory_inodeList;
	bool		rm_modes[rm_mode_Num];

	char 		*source_path;
	char 		 source_name[(info->sB).filenameSize];
	int 		 source_nodeid;
	MDS 		*source_mds;

	int 		 directory_nodeid;

	char		 destination_path[strlen(destination) + (info->sB).filenameSize + 2];
	char 		 destination_name[(info->sB).filenameSize];
	int			 destination_nodeid;
	MDS 		*destination_mds;

	for (int i=0; i<rm_mode_Num; i++)
		rm_modes[i] = false;
	//------------------------------------------------------------------------------------
	// Get rid of possible extra '/'
	cleanSlashes(&destination);
	// Get destination's directory inode
	directory_nodeid = get_parent(info, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("mv: cannot create directory '%s': No such file or directory\n", destination);
		return false;
	}
	//------------------------------------------------------------------------------------
	// Move each source to destination.
	while (sourceList != NULL) {
		source_path = pop_last_string(&sourceList);
		// Get rid of possible extra '/'
		cleanSlashes(&source_path);
		// Get source inode.
		get_parent(info, source_path, source_name);
		source_nodeid = traverse_cfs(info, source_path);
		// Check if source_path has valid value.
		if(source_nodeid == -1) {
			printf("mv: cannot stat '%s': No such file or directory\n",source_path);
			free(source_path);
			continue;
		}
		source_mds = (MDS*) (info->inodeTable + source_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
		source_mds->access_time = time(NULL);
		//--------------------------------------------------------------------------------
		// Get destination inode.
		destination_nodeid = traverse_cfs(info,destination_name);
		strcpy(destination_path, destination);
		if (destination_nodeid == -1) {
			// If the destination does not exists, it has to be imported only one source.
			// Create a new inode at the destination and copy the source in it.
			if (sourcesCount != 1) {
				printf("mv: target '%s' is not a directory\n", destination);
			} else if (source_mds->type == Directory){
				directory_inodeList = NULL;
				getDirEntities(info, source_path, &directory_inodeList);
				cfs_mkdir(info, destination_path);
				cfs_mv(info, mv_modes, directory_inodeList, destination_path);
			} else {	// source_mds->type == File.
				cfs_touch(info, destination_path, CRE);
				destination_nodeid = traverse_cfs(info, destination_path);
				replaceEntity(info, source_nodeid, destination_nodeid);
			}
		} else {	// destination_nodeid != -1.
			// If the destination exists check its type.
			destination_mds = (MDS*) (info->inodeTable + destination_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			destination_mds->access_time = time(NULL);
			if (destination_mds->type == Directory) {
				// If destination is a directory check if exists the destination/source_name.
				strcat(destination_path, "/");
				strcat(destination_path, source_name);
				destination_nodeid = traverse_cfs(info,source_name);
				if (destination_nodeid == -1) {
					// If destination/source_name does not exists create a new directory or file with name <source_name>.
					if(source_mds->type == Directory) {
						directory_inodeList = NULL;
						getDirEntities(info, source_path, &directory_inodeList);
						cfs_mkdir(info, destination_path);
						cfs_mv(info, mv_modes, directory_inodeList, destination_path);
					} else {
						cfs_touch(info, destination_path, CRE);
						destination_nodeid = traverse_cfs(info, destination_path);
						replaceEntity(info, source_nodeid, destination_nodeid);
					}
				} else {	// destination_nodeid != -1.
					// If destination/source_name exists check if the type of source and destination are matching.
					destination_mds = (MDS*) (info->inodeTable + destination_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
					destination_mds->access_time = time(NULL);
					if (source_mds->type == Directory && destination_mds->type == Directory) {
						if (mv_modes[MV_I] == true) {
							printf("mv: You want to overide '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Move denied.\n");
								free(source_path);
								cfs_rm(info, rm_modes, source_path);
								continue;
							}
						}
						directory_inodeList = NULL;
						getDirEntities(info, source_path, &directory_inodeList);
						cfs_mv(info, mv_modes, directory_inodeList, destination_path);
					} else if (source_mds->type == File && destination_mds->type == File) {
						if (mv_modes[MV_I] == true) {
							printf("mv: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Move denied.\n");
								cfs_rm(info, rm_modes, source_path);
								free(source_path);
								continue;
							}
						}
						current_time = time(NULL);
						destination_mds->creation_time = current_time;
						destination_mds->access_time = current_time;
						destination_mds->modification_time = current_time;
						replaceEntity(info, source_nodeid, destination_nodeid);
						destination_mds->linkCounter = 1;
					} else if (source_mds->type != Directory && destination_mds->type == Directory) {
						printf("mv: cannot overwrite directory '%s' with non-directory '%s'\n", destination_path, source_path);
					} else if (source_mds->type == Directory && destination_mds->type != Directory) {
						printf("mv: cannot overwrite non-directory '%s' with directory '%s'\n", destination_path, source_path);
					}
				}
			} else {	// destination_mds->type == File.
				// If destination is not a directory, first check the sourcesCount and after the source type.
				if (sourcesCount != 1) {
					printf("mv: target '%s' is not a directory\n", destination);
				} else if (source_mds->type == Directory) {
					printf("mv: cannot overwrite non-directory '%s' with directory '%s'\n", destination, source_path);
				} else if (source_mds->type == File) {
					if (mv_modes[MV_I] == true) {
						printf("mv: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
						fgets(line, 100, stdin);
						if (line != NULL){
							answer = strtok(line, " \n\t");
						}
						if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
							printf("Move denied.\n");
							cfs_rm(info, rm_modes, source_path);
							free(source_path);
							continue;
						}
					}
					current_time = time(NULL);
					destination_mds->creation_time = current_time;
					destination_mds->access_time = current_time;
					destination_mds->modification_time = current_time;
					replaceEntity(info, source_nodeid, destination_nodeid);
					destination_mds->linkCounter = 1;
				}
			}
		}
		cfs_rm(info, rm_modes, source_path);
		free(source_path);
	}

	return true;
}

bool cfs_rm(cfs_info *info, bool *modes, char *dirname)
{
	int		move, ignore = 0;
	int		nodeid;
	int		dataCounter, checked = 0;
	MDS		*metadata;
	Datastream	data;

	char		line[100], *answer;
	char		content_name[(info->sB).filenameSize];
	int		content_nodeid, newCounter;
	MDS		*content_mds;
	Datastream	content_data;

	// Get rid of possible extra '/'
	cleanSlashes(&dirname);
	// Get the nodeid of the entity the destination path leads to
	nodeid = traverse_cfs(info,dirname);
	// If destination parth is invalid
	if(nodeid == -1)
	{
		printf("rm: Path %s could not be found in cfs.\n",dirname);
		return false;
	}

	metadata = (MDS*) (info->inodeTable + nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	metadata->access_time = time(NULL);
	data.datablocks = (int*) (info->inodeTable + nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));
	// If it is not a directory
	if(metadata->type != Directory)
	{
		printf("rm: Input error, %s is not a directory.\n",dirname);
		return false;
	}

	// For directory's each datablock
	for(int i=0; i<(info->sB).maxFileDatablockNum; i++)
	{
		// If datablock has contents
		if(data.datablocks[i] != -1)
		{	// Get number of entities in current datablock
			CALL(lseek(info->fileDesc,data.datablocks[i]*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			// Number of entities in datablock after remove is completed
			newCounter = 0;
			// For every entity in datablock
			for(int j=0; j<dataCounter; j++)
			{
				move = data.datablocks[i]*(info->sB).blockSize + sizeof(int) + j*((info->sB).filenameSize + sizeof(int));
				CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				// Get current entity's name
				SAFE_READ(info->fileDesc,content_name,0,sizeof(char),(info->sB).filenameSize);
				// If it is not the current or parent entity
				if(strcmp(content_name,".") && strcmp(content_name,".."))
				{	// Get entity's nodeid
					SAFE_READ(info->fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));

					// If '-i' was given as an argument
					if(modes[RM_I] == true)
					{	// Ask user for confirmation
						printf("rm: Are you sure you want to delete item '%s' ? (Press y|Y for yes) ",content_name);
						fgets(line,100,stdin);
						if(line != NULL)
							answer = strtok(line," \n\t");

						// If user said no, go to the next entity
						if(answer == NULL || strcmp(answer,"y") || strcmp(answer,"Y"))
						{
							// If there is a hole inside datablock, fill it
							if(j > newCounter)
							{
								move = data.datablocks[i]*(info->sB).blockSize + sizeof(int) + newCounter*((info->sB).filenameSize+sizeof(int));
								CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
								SAFE_WRITE(info->fileDesc,content_name,0,sizeof(char),(info->sB).filenameSize);
								SAFE_WRITE(info->fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));
							}
							// Increase newCounter and go to the next entity
							newCounter++;
							continue;
						}
					}

					content_mds = (MDS*) (info->inodeTable + content_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
					content_mds->access_time = time(NULL);
					content_data.datablocks = (int*) (info->inodeTable + content_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS));
					// If it is a file, it will be removed
					if(content_mds->type == File)
					{
						content_mds->linkCounter--;
						content_mds->modification_time = time(NULL);
						if(!content_mds->linkCounter)
						{
							// Push allocated datablocks in holeList
							for(int k=0; k<(info->sB).maxFileDatablockNum; k++)
								if(content_data.datablocks[k] != -1)
								{
									addNode(&(info->holes),content_data.datablocks[k]);
									(info->sB).ListSize++;
								}
						}
					}
					else	// If it is a directory
					{	// If it has contnents
						if(content_mds->datablocksCounter != 0)
						{	// If '-r' was given as an argument, it will be removed
							if(modes[RM_R] == true)
							{
								char	recursion_name[strlen(dirname)+(info->sB).filenameSize+2];
								strcpy(recursion_name,dirname);
								strcat(recursion_name,"/");
								strcat(recursion_name,content_name);
								cfs_rm(info,modes,recursion_name);
								// Remove directory's first datablock (with the current and parent entities)
								addNode(&(info->holes),content_data.datablocks[0]);
								(info->sB).ListSize++;
							}
							// Else, it will not be removed. If there is a hole inside datablock, fill it
							else if(j > newCounter)
							{
								move = data.datablocks[i]*(info->sB).blockSize + sizeof(int) + newCounter*((info->sB).filenameSize+sizeof(int));
								CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
								SAFE_WRITE(info->fileDesc,content_name,0,sizeof(char),(info->sB).filenameSize);
								SAFE_WRITE(info->fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));
								printf("rm: cannot remove '%s': Is a non-empty directory", dirname);
								// Increase newCounter and go to the next entity
								newCounter++;
								continue;
							}
							else
							{
								printf("rm: cannot remove '%s': Is a non-empty directory", dirname);
								newCounter++;
								continue;
							}
						}
					}
					// If it is a file or an empty directory, remove it
					*(bool*) (info->inodeTable + content_nodeid*(info->inodeSize)) = false;
					metadata->size -= (info->sB).filenameSize + sizeof(int);
					metadata->modification_time = time(NULL);
					(info->sB).nodeidCounter--;
					// If it is the last in inodeTable do not keep the hole (inner holes should be maintained)
					if(content_nodeid == ((info->sB).iTableCounter-1))
						(info->sB).iTableCounter--;
				}
				// If it is the current or parent entity
				else
					newCounter++;
			}
			// If datablock has still contents (maybe only current and parent entity), update counter
			if(newCounter > 0)
			{
				CALL(lseek(info->fileDesc,data.datablocks[i]*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(info->fileDesc,&newCounter,0,sizeof(int),sizeof(int));
			}
			// Else, add it to holeList and remove it from directory's data
			else
			{
				addNode(&(info->holes),data.datablocks[i]);
				(info->sB).ListSize++;
				data.datablocks[i] = -1;
				metadata->datablocksCounter--;
				metadata->modification_time = time(NULL);
				checked--;
			}

			checked++;
		}

		// If all directory's contents have been checked
		if(checked == metadata->datablocksCounter)
			break;
	}

	return true;
}

bool cfs_cat(cfs_info *info, string_List *sourceList, char *outputPath)
{
	unsigned int	total_size = 0, output_size;
	int		source_nodeid, output_nodeid;
	MDS		*source_mds, *output_mds;

	int	listSize, i;
	char	*source_name = NULL;

	// Get rid of possible extra '/'
	cleanSlashes(&outputPath);
	// Get the nodeid of the entity the output path leads to
	output_nodeid = traverse_cfs(info,outputPath);
	// If output path was invalid
	if(output_nodeid == -1)
	{
		printf("cat: Path %s could not be found in cfs.\n",outputPath);
		return false;
	}
	// Get output file's size
	output_mds = (MDS*) (info->inodeTable + output_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
	output_mds->access_time = time(NULL);
	if(output_mds->type != File)
	{
		printf("cat: Input error, output path %s is not a file.\n",info->inodeTable+output_nodeid*(info->inodeSize)+sizeof(bool));
		return false;
	}
	output_size = output_mds->size;

	// Get number of files in sourceList
	listSize = getLength(sourceList);
	// For every source file
	for(i=1; i<=listSize; i++)
	{
		source_name = get_stringNode(&sourceList,i);
		if(source_name != NULL)
		{
			// Get rid of possible extra '/'
			cleanSlashes(&source_name);
			source_nodeid = traverse_cfs(info,source_name);
			if(source_nodeid == -1)
			{
				printf("cat: Path %s could not be found in cfs.\n",source_name);
				return false;
			}
			source_mds = (MDS*) (info->inodeTable + source_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			source_mds->access_time = time(NULL);
			if(source_mds->type != File)
			{
				printf("cat: Input error, source path %s is not a file.\n",info->inodeTable+source_nodeid*(info->inodeSize)+sizeof(bool));
				return false;
			}
			total_size += source_mds->size;
		}
		else
			break;
	}
	// If less files than expected were found
	if(i <= listSize)
	{
		printf("cat: Input error, problem with list of source files.\n");
		return false;
	}

	if(total_size > ((info->sB).maxFileSize - output_size))
	{
		printf("cat: Not enough space in output file to cat all source files.\n");
		return false;
	}

	for(int i=0; i<listSize; i++)
	{
		source_name = pop_last_string(&sourceList);
		source_nodeid = traverse_cfs(info,source_name);
		if(source_nodeid == -1)
		{
			printf("cat: Path %s could not be found in cfs.\n",source_name);
			free(source_name);
			return false;
		}
		append_file(info,source_nodeid,output_nodeid);
		free(source_name);
	}

	return true;
}

bool cfs_import(cfs_info *info,string_List *sourceList,char *destPath)
{
	int		ignore = 0;
	int		source_nodeid, dest_nodeid, dest_entities;
	char		*initial;
	char		new_path[strlen(destPath)+((info->sB).filenameSize+2)];
	MDS		*source_mds, *dest_mds;
	Datastream	data;

	int	listSize, i;
	char	*source_name = NULL;

	int		move;
	char		fileBuffer[((info->sB).blockSize)];
	char		*split, *temp;
	struct stat	entity;

	// Get rid of possible extra '/'
	cleanSlashes(&destPath);
	// Get the nodeid of the entity the destination path leads to
	dest_nodeid = traverse_cfs(info,destPath);
	// If destination path was invalid
	if(dest_nodeid == -1)
	{
		printf("import: Path %s could not be found in cfs.\n",destPath);
		return false;
	}
	// Get destination directory's metadata
	dest_mds = (MDS*) ((info->inodeTable) + dest_nodeid*(info->inodeSize) + sizeof(bool) + ((info->sB).filenameSize));
	if(dest_mds->type != Directory)
	{
		printf("import: Input error, destination path %s is not a directory.\n",(info->inodeTable)+dest_nodeid*(info->inodeSize)+sizeof(bool));
		return false;
	}
	dest_entities = getDirEntities(info,destPath,NULL);

	// Get number of entities in sourceList
	listSize = getLength(sourceList);
	// If source entities require too much space
	if(listSize > (((info->sB).maxFilesPerDir) - dest_entities))
	{
		printf("import: Error, not enough space in destination directory.\n");
		return false;
	}

	// Check if list with source files is valid
	for(i=1; i<=listSize; i++)
	{
		source_name = get_stringNode(&sourceList,i);
		// If source_name is invalid
		if(source_name == NULL)
		{
			printf("import: Input error, problem with list of source files.\n");
			return false;
		}
	}

	// For every source file
	for(i=0; i<listSize; i++)
	{
		source_name = pop_last_string(&sourceList);
		if(source_name != NULL)
		{
			// Get rid of possible extra '/'
			cleanSlashes(&source_name);
			// If source entity does not exist
			if(access(source_name,F_OK) == -1)
			{
				printf("import: Input error, source entity %s does not exist.\n",source_name);
				free(source_name);
				return false;
			}
			// New entity will be in destination directory
			initial = (char*)malloc(strlen(source_name)+1);
			strcpy(initial,source_name);
			split = strtok(initial,"/");
			temp = strtok(NULL,"/");
			while(temp != NULL)
			{
				split = temp;
				temp = strtok(NULL,"/");
			}
			strcpy(new_path,destPath);
			strcat(new_path,"/");
			strcat(new_path,split);
			free(initial);

			// Try to get source's type
			if(stat(source_name,&entity) == -1)
			{
				perror("Error with stat in source entity: ");
				free(source_name);
				return false;
			}
			// If it is a directory
			if(S_ISDIR(entity.st_mode))
			{
				DIR		*dir;
				struct dirent	*content;
				char		content_path[strlen(source_name)+((info->sB).filenameSize+2)];

				if((dir = opendir(source_name)) == NULL)
				{
					perror("Error opening source directory: ");
					free(source_name);
					return false;
				}

				// Create a new directory in cfs (under the appropriate parent directory)
				source_nodeid = cfs_mkdir(info,new_path);
				if(source_nodeid == -1)
				{
					free(source_name);
					return false;
				}

				string_List	*contentList = NULL;
				while((content = readdir(dir)) != NULL)
				{
					if(strcmp(content->d_name,".") && strcmp(content->d_name,".."))
					{
						strcpy(content_path,source_name);
						strcat(content_path,"/");
						strcat(content_path,content->d_name);
						add_stringNode(&contentList,content_path);
						cfs_import(info,contentList,new_path);
						contentList = NULL;
					}
				}
				closedir(dir);
			}
			// If it is a file
			else if(S_ISREG(entity.st_mode))
			{
				int	source_fd, size_to_read, source_end, current;

				if((source_fd = open(source_name, O_RDONLY)) == -1)
				{
					perror("Error opening source file: ");
					free(source_name);
					return false;
				}

				// Create a new file in cfs (under the appropriate parent directory)
				source_nodeid = cfs_touch(info,new_path,CRE);
				if(source_nodeid == -1)
				{
//					free(source_name);
					return false;
				}
				move = source_nodeid*(info->inodeSize) + sizeof(bool) + ((info->sB).filenameSize);
				source_mds = (MDS*) (info->inodeTable + move);
				move += sizeof(MDS);
				data.datablocks = (int*) (info->inodeTable + move);

				// Get file's size
				source_end = entity.st_size;
				size_to_read = (((info->sB).blockSize) > source_end) ? source_end : ((info->sB).blockSize);
				// Go to the beginning and read file's contents
				CALL(lseek(source_fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(source_fd,fileBuffer,0,sizeof(char),size_to_read);

				// Fill file's data
				for(int j=0; j<(((info->sB).maxFileDatablockNum)); j++)
				{
					data.datablocks[j] = getEmptyBlock(info);
					// File's datablocks are increased by one
					source_mds->datablocksCounter++;
					move = data.datablocks[j]*((info->sB).blockSize);
					CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					// Write source file's contents
					SAFE_WRITE(info->fileDesc,fileBuffer,0,sizeof(char),size_to_read);

					// Read source file's next contents
					// Get current location in source directory
					CALL(lseek(source_fd,0,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,current);
					// If there are still info to read
					if(current < source_end)
					{
						size_to_read = (((info->sB).blockSize) > (source_end-current)) ? (source_end-current) : (((info->sB).blockSize));
						SAFE_READ(source_fd,fileBuffer,0,sizeof(char),size_to_read);
					}
					// Reached end of directory
					else
						break;
				}
				close(source_fd);
				source_mds->size += source_end;
			}
			free(source_name);
		}
			
		else
			break;
	}
	// If less files than expected were found
	if(i < listSize)
	{
		printf("import: Input error, problem with list of source files.\n");
		return false;
	}

	return true;
}

bool cfs_export(cfs_info *info,string_List *sourceList,char *destPath)
{
	int		ignore = 0;
	int		source_nodeid;
	char		*initial;
	char		new_path[strlen(destPath)+((info->sB).filenameSize+2)];
	MDS		*source_mds;
	struct stat	entity;
	char		*split,*temp;

	int	listSize, i;
	char	*source_name = NULL;

	// Get rid of possible extra '/'
	cleanSlashes(&destPath);
	// If destination path does not exist
	if(access(destPath,F_OK) == -1)
	{
		printf("export: Input error, destination directory %s does not exist.\n",destPath);
		return false;
	}

	// Try to get destination's type
	if(stat(destPath,&entity) == -1)
	{
		perror("export: Error with stat in destination entity: ");
		return false;
	}
	// If destination is not a directory
	if(!S_ISDIR(entity.st_mode))
	{
		printf("export: Input error, destination path %s is not a directory.\n",destPath);
		return false;
	}

	// Get number of entities in sourceList
	listSize = getLength(sourceList);
	// Check if list with source files is valid
	for(i=1; i<=listSize; i++)
	{
		source_name = get_stringNode(&sourceList,i);
		// If source_name is invalid
		if(source_name == NULL)
		{
			printf("export: Input error, problem with list of source files.\n");
			return false;
		}
	}

	// For every source file
	for(i=0; i<listSize; i++)
	{
		source_name = pop_last_string(&sourceList);
		if(source_name != NULL)
		{
			// If it is '.' or '..' entity, ignore it
			if(!strcmp(source_name,".") || !strcmp(source_name,".."))
			{
				free(source_name);
				continue;
			}

			// Get rid of possible extra '/'
			cleanSlashes(&source_name);
			// Get the nodeid of the entity the source path leads to
			source_nodeid = traverse_cfs(info,source_name);
			// If source path was invalid
			if(source_nodeid == -1)
			{
				printf("export: Path %s could not be found in cfs.\n",source_name);
				free(source_name);
				return false;
			}
			// New entity will be in destination directory
			initial = (char*)malloc(strlen(source_name)+1);
			strcpy(initial,source_name);
			split = strtok(initial,"/");
			temp = strtok(NULL,"/");
			while(temp != NULL)
			{
				split = temp;
				temp = strtok(NULL,"/");
			}
			strcpy(new_path,destPath);
			strcat(new_path,"/");
			// If current entity is the root, new directory's name will be the name of the cfs file
			if(!strcmp(split,"/"))
				strcat(new_path,info->fileName);
			else
				strcat(new_path,split);
			free(initial);

			// Get source's metadata
			source_mds = (MDS*) ((info->inodeTable) + source_nodeid*(info->inodeSize) + sizeof(bool) + ((info->sB).filenameSize));
			// If it is a directory
			if(source_mds->type == Directory)
			{
				// Create subdirectory
				CALL(mkdir(new_path, S_IRWXU | S_IXGRP),-1,"Error creating directory from cfs: ",1,ignore);
				// Get subdirectory's contents and export them
				string_List	*contentList = NULL;
				getDirEntities(info,source_name,&contentList);
				cfs_export(info,contentList,new_path);
			}
			// If it is a file
			else
			{
				int		move;
				int		source_fd;
				char		fileBuffer[(info->sB).blockSize];
				Datastream	data;

				// Create file
				CALL(creat(new_path, S_IRWXU | S_IXGRP),-1,"Error creating file from cfs: ",1,source_fd);

				move = source_nodeid*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize + sizeof(MDS);
				data.datablocks = (int*) (info->inodeTable + move);

				for(int i=0; i<((info->sB).maxFileDatablockNum); i++)
				{
					if(data.datablocks[i] != -1)
					{
						move = data.datablocks[i]*((info->sB).blockSize);
						CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(info->fileDesc,fileBuffer,0,sizeof(char),(info->sB).blockSize);
						SAFE_WRITE(source_fd,fileBuffer,0,sizeof(char),(info->sB).blockSize);
					}
				}

				close(source_fd);
			}

			free(source_name);
		}

		else
			break;
	}

	// If less files than expected were found
	if(i < listSize)
	{
		printf("export: Input error, problem with list of source files.\n");
		return false;
	}

	return true;
}

int cfs_create(cfs_info *info, int bSize,int nameSize,unsigned int maxFSize,int maxDirFileNum)
{
	int		ignore = 0;
	time_t		current_time;
	MDS		metadata;
	Datastream	data;

	info->holes = NULL;

	CALL(creat(info->fileName, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,info->fileDesc);

	// Create superblock
	// Block size has to be a multiple of 512 bytes
	if(bSize != -1 && (bSize % BLOCK_SIZE))
	{
		printf("Input error, block size is too small. Please try again.\n");
		return -1;
	}
	(info->sB).blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	// Filename size has a ceiling (252 bytes)
	if(nameSize != -1 && nameSize > MAX_FILENAME_SIZE)
	{
		printf("Input error, filename size is too long. Please try again.\n");
		return -1;
	}
	(info->sB).filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	(info->sB).maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	(info->sB).maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	///	(info->sB).metadataBlocksNum = ((info->sB).filenameSize + sizeof(MDS) + ((info->sB).maxDatablockNum)*sizeof(unsigned int)) / (info->sB).blockSize;
	///	if((((info->sB).filenameSize + sizeof(MDS) + ((info->sB).maxDatablockNum)*sizeof(unsigned int)) % (info->sB).blockSize) > 0)
	///        	(info->sB).metadataBlocksNum++;
	// Every datablock of a directory contains some pairs of (filename+nodeid) for the directory's contents and a counter of those pairs
	(info->sB).maxEntitiesPerBlock = ((info->sB).blockSize - sizeof(int)) / ((info->sB).filenameSize + sizeof(int));
	(info->sB).maxFileDatablockNum = ((info->sB).maxFileSize) / ((info->sB).blockSize);
	//	if(((info->sB).blockSize - sizeof(int)) % ((info->sB).filenameSize + sizeof(int)))
	if((info->sB).maxFileSize % (info->sB).blockSize)
		(info->sB).maxFileDatablockNum++;
	(info->sB).maxDirDatablockNum = ((info->sB).maxFilesPerDir + 2) / (info->sB).maxEntitiesPerBlock;
	if(((info->sB).maxFilesPerDir + 2) % (info->sB).maxEntitiesPerBlock)
		(info->sB).maxDirDatablockNum++;

	// Root's nodeid
	(info->sB).root = 0;
	// Assume block-counting is zero based	
	(info->sB).blockCounter = 0;
	// Only root in cfs for now
	(info->sB).nodeidCounter = 1;
	(info->sB).ListSize = 0;
	// Assume that root's metadata can be stored in one block for now ((info->sB).filenameSize+sizeof(MDS)+sizeof(int))
	(info->sB).iTableBlocksNum = 1;
	(info->sB).iTableCounter = (info->sB).nodeidCounter;
	// Get superblock's overflow block (contains the inodeTable and the holeList)
	(info->sB).nextSuperBlock = getEmptyBlock(info);
	CALL(lseek(info->fileDesc,(info->sB).nextSuperBlock*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	int	overflow_block = -1;
	SAFE_WRITE(info->fileDesc,&overflow_block,0,sizeof(int),sizeof(int));

	int	blocknum;
	// Get block for root's meatdata (one block for now)
	blocknum = getEmptyBlock(info);
	// Write blocknum in iTable map
	SAFE_WRITE(info->fileDesc,&blocknum,0,sizeof(int),sizeof(int));

	// Write superBlock	
	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(info->sB),0,sizeof(superBlock),sizeof(superBlock));

	// Go to block with root's metadata (one block for now)
	CALL(lseek(info->fileDesc,blocknum*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	// Write root's name (cannot be larger than a block)
	CALL(write(info->fileDesc,"/",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(info->fileDesc,(info->sB).filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);

	metadata.nodeid = 0;
	// Root's data contain the relative paths (. & ..)
	metadata.size = 2*((info->sB).filenameSize + sizeof(int));
	metadata.type = Directory;
	metadata.parent_nodeid = 0;
	current_time = time(NULL);
	metadata.creation_time = current_time;
	metadata.access_time = current_time;
	metadata.modification_time = current_time;
	// Entities for . and .. can be stored in one block
	metadata.datablocksCounter = 1;

	// Write root's metadata
	SAFE_WRITE(info->fileDesc,&metadata,0,sizeof(MDS),sizeof(MDS));

	// Create root's data blocks (one block for now)
	data.datablocks = (int*)malloc(sizeof(int));
	data.datablocks[0] = getEmptyBlock(info);
	CALL(lseek(info->fileDesc,data.datablocks[0]*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	// Write entities' counter for current datablock
	int	entityCounter = 2;
	SAFE_WRITE(info->fileDesc,&entityCounter,0,sizeof(int),sizeof(int));
	// Write entity (filename + nodeid) for relative path "."
	CALL(write(info->fileDesc,".",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(info->fileDesc,(info->sB).filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(metadata.nodeid),0,sizeof(int),sizeof(int));
	// Write entity for relative path ".."
	CALL(write(info->fileDesc,"..",3),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(info->fileDesc,(info->sB).filenameSize-3,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(metadata.parent_nodeid),0,sizeof(int),sizeof(int));

	// Write root's datablocks ~part of metadata~ (one block for now)
	int	move = blocknum*(info->sB).blockSize + (info->sB).filenameSize + sizeof(MDS);
	CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,data.datablocks,0,sizeof(int),sizeof(int));

	// Write superBlock	
	CALL(lseek(info->fileDesc,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(info->fileDesc,&(info->sB),0,sizeof(superBlock),sizeof(superBlock));

	CALL(close(info->fileDesc),-1,"Error closing file for cfs: ",4,ignore);

	free(data.datablocks);
	return info->fileDesc;
}

bool cfs_close(cfs_info *info) 
{
	if(info->fileDesc != -1)
	{
		int ignore = 0;
		// Update superblock, inodeTable, holeList
        	update_superBlock(info);
        	free(info->inodeTable);

        	CALL(close(info->fileDesc),-1,"Error closing file for cfs: ",4,ignore);
        	return true;
    	}

	return false;
}
