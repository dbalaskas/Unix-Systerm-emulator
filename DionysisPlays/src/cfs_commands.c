/* FILE: cfs_commands.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include <dirent.h>
#include "../include/cfs_commands.h"

int cfs_workwith(char *filename)
{
	int	fd = -1, ignore = 0;
	int	offset = 0, listOffset;
	int	overflow_block, read_iblocks, metadata_blocknum;
	int	current_iblock[sB.iTableBlocksNum];
	MDS	*metadata;

	// CALL(open(filename, O_RDWR),-1,"Error opening file for cfs: ",2,fd);
	if((fd = open(filename, O_RDWR)) == -1) {
		perror("Error opening file for cfs: ");
		return -1;
	}
	// We assume that we are in root at first, update global variable of our location
	cfs_current_nodeid = 0;

	// Go to the first block, read superBlock
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_READ(fd,&sB,0,sizeof(superBlock),sizeof(superBlock));

	// (first) Block with the inodeTable and the holeList
	overflow_block = sB.nextSuperBlock;
	read_iblocks = 0;

	// Size of an inodeTable element
	inodeSize = sizeof(bool) + sB.filenameSize + sizeof(MDS) + (sB.maxFileDatablockNum)*sizeof(int);
	// Buffer to store info of an inodeTable element
	char	buffer[inodeSize-sizeof(bool)];
        int	size_to_read, dataSize, readSize;
	int	remainingDatablocks;
        int	*nodeId;
        int	numOfiNode = 0;
        inodeTable = (char*)malloc(sB.iTableCounter*inodeSize);
        holes = NULL;

	// For every element of the inodeTable
	for(int j=0; j<sB.nodeidCounter; j++)
	{
		// Start from the first block with the element's metadata
		metadata_blocknum = 0;

		// Will go to (the appropriate) block used by the inodeTable
		read_iblocks += sizeof(int);
		// Go to (the appropriate) block with the inodeTable
		CALL(lseek(fd,(overflow_block*sB.blockSize)+read_iblocks,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		// current_iblock[metadata] = appropriate block with metadata
		SAFE_READ(fd,current_iblock,metadata_blocknum*sizeof(int),sizeof(int),sizeof(int));
		// Go to (the appropriate) block with metadata
        	CALL(lseek(fd,current_iblock[metadata_blocknum]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		// Will read filename and MDS struct for now (stored easily in only one block)
		size_to_read = sB.filenameSize + sizeof(MDS);
		SAFE_READ(fd,buffer,metadata_blocknum*sB.blockSize,sizeof(char),size_to_read);
		readSize = size_to_read;

		// Get datablocksCounter (number of datablocks) from metadata
		metadata = (MDS*) (buffer + sB.filenameSize);
		dataSize = (metadata->datablocksCounter)*sizeof(int);
		// Rest of datablocks in inodeTable that will be empty for now
		remainingDatablocks = sB.maxFileDatablockNum - metadata->datablocksCounter;
		// Read all datablocks with contents (from the appropriate blocks)
		size_to_read = (dataSize > (sB.blockSize-readSize)) ? (sB.blockSize-readSize) : (dataSize);
		// size_to_read must be a multiple of sizeof(int)
		if(size_to_read % sizeof(int))
			size_to_read -= size_to_read % sizeof(int);

		while(dataSize > 0)
		{
			SAFE_READ(fd,buffer,(metadata_blocknum*sB.blockSize)+readSize,sizeof(char),size_to_read);
			dataSize -= size_to_read;

			// If there are more metadata in another block
			if(dataSize > 0)
			{
				// Go to the next block of metadata
				metadata_blocknum++;
				// Go to the next block used by the inodeTable
				read_iblocks += sizeof(int);

				// If inodeTable has more elements, it is continued to another block (overflow)
				if(read_iblocks == sB.blockSize)
				{
					// Get overflow block
					CALL(lseek(fd,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(fd,&overflow_block,0,sizeof(int),sizeof(int));
					read_iblocks = sizeof(int);
				}

				// Go to (the appropriate) block with the inodeTable
				CALL(lseek(fd,(overflow_block*sB.blockSize)+read_iblocks,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				// current_iblock[metadata] = appropriate block with metadata
				SAFE_READ(fd,current_iblock,metadata_blocknum*sizeof(int),sizeof(int),sizeof(int));
				// Go to (the appropriate) block with metadata
		        	CALL(lseek(fd,current_iblock[metadata_blocknum]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				readSize = 0;
				size_to_read = (dataSize > (sB.blockSize-readSize)) ? (sB.blockSize-readSize) : (dataSize);
				// size_to_read must be a multiple of sizeof(int)
				if(size_to_read % sizeof(int))
					size_to_read -= size_to_read % sizeof(int);
			}
		}

	        nodeId = (int*) (buffer + sB.filenameSize);				//update inodeTable
        	// Find the right place for the inode ~based on nodeid~ (holes in the inodeTable have to be maintained)
            	while(numOfiNode < *nodeId)
	        {
        	       	*(inodeTable + offset) = false;                                 //empty
                	numOfiNode++;
                	offset += inodeSize;
	        }

		*(bool*)(inodeTable + offset) = true;
		offset += sizeof(bool);
		// Copy inodeTable from cfs file to table structure
		memcpy(inodeTable+offset, buffer, inodeSize-sizeof(bool)-(remainingDatablocks*sizeof(int)));
		int	number = -1;
		offset += inodeSize - sizeof(bool) - (remainingDatablocks*sizeof(int));
		for(int k=0; k<remainingDatablocks; k++)
		{
			memcpy(inodeTable+offset, &number , sizeof(int));
			offset += sizeof(int);
		}

		// Go to inodeTable's next element
	        numOfiNode++;
	}

	int	difference = sB.iTableBlocksNum*sizeof(int);
	int	overflow_prev, overflow_next;
	int	remainingblockSize = sB.blockSize - sizeof(int);
	// Start from first block with the inodeTable
	overflow_block = sB.nextSuperBlock;
	// Find the end of the inodeTable
	while(difference > 0)
	{
		CALL(lseek(fd,overflow_block*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		overflow_prev = overflow_block;
		SAFE_READ(fd,&overflow_block,0,sizeof(int),sizeof(int));

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

	int	blockNum;
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
				SAFE_WRITE(fd,&overflow_next,0,sizeof(int),sizeof(int));
			}
			// If an empty block is needed
			else
			{
				overflow_block = getEmptyBlock();
				listOffset = overflow_block*sB.blockSize;
	            		CALL(lseek(fd,listOffset,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		            		overflow_next = -1;
				SAFE_WRITE(fd,&overflow_next,0,sizeof(int),sizeof(int));
			}
				freeSpaces = remainingblockSize;
		}
		// Read a list node (fileptr is on the right spot)
		SAFE_READ(fd,&blockNum,0,sizeof(int),sizeof(int));
		// Push it in List structure
       		addNode(&holes, blockNum);
		// Space in current block is reduced by one list node
		freeSpaces -= sizeof(int);
	}

	return fd;
}

int cfs_mkdir(int fd,char *dirname)
{
	int		ignore = 0;
	int		new_nodeid;
	int		parent_nodeid, i, offset;
	char	parent_path[strlen(dirname)+1];
	char		new_name[sB.filenameSize+1];
	bool		busy;
	time_t		curr_time;
	MDS		*parent_mds, *metadata;
	Datastream	parent_data, data;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		
	// Initialize new_name
	for(int i=0; i<=sB.filenameSize; i++)
		new_name[i] = '\0';

	// Get parent's nodeid and new directory's name
	parent_nodeid = get_parent(fd,dirname,new_name);
	if(parent_nodeid == -1)
		return -1;
	// Go to parent's metadata
	parent_mds = (MDS*) (inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	// Check if parent is a directory
	if(parent_mds->type != Directory)
	{
		printf("Input error, %s is not a directory.\n",inodeTable+parent_nodeid*inodeSize+sizeof(bool));
		return -1;
	}

	// Check if new directory exists
	if(traverse_cfs(fd,new_name,parent_nodeid) != -1)
	{
		printf("Input error, directory %s already exists.\n",dirname);
		return -1;
	}

	// Go to parent's data
	parent_data.datablocks = (int*)(inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

	int	dataCounter, move, numofEntities;
	if (strlen(dirname)-strlen(new_name) > 0) {
		strncpy(parent_path, dirname, strlen(dirname)-strlen(new_name));
		parent_path[strlen(dirname)-strlen(new_name)] = '\0';
	} else {
		strcpy(parent_path, "./");
	}
	numofEntities = getDirEntities(fd,parent_path,NULL);

	// Update parent directory's data
	for(i=0; i<sB.maxFileDatablockNum; i++)
	{
		// If current datablock is empty
		if(parent_data.datablocks[i] == -1)
		{
			parent_data.datablocks[i] = getEmptyBlock();
			// Go to datablock
			move = parent_data.datablocks[i]*sB.blockSize + sizeof(int);
			CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			dataCounter = 0;
			parent_mds->datablocksCounter++;
		}
		else
		{
			// Go to datablock
			move = parent_data.datablocks[i]*sB.blockSize;
			CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(fd,&dataCounter,0,sizeof(int),sizeof(int));
		}

		// If parent directory has no datablocks available
		if(parent_mds->datablocksCounter == sB.maxDirDatablockNum && numofEntities == sB.maxEntitiesPerBlock*sB.maxDirDatablockNum)
		{
			printf("Not enough space in parent directory to create subdirectory %s.\n",dirname);
			return -1;
		}
	
		// If there is space for another entity
		if(dataCounter < sB.maxEntitiesPerBlock)
		{
			// Find an empty space in inodeTable
			new_nodeid = getTableSpace();

			// Go after the last entity in the datablockrealloc
			CALL(lseek(fd,dataCounter*(sB.filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(fd,new_name,0,sizeof(char),sB.filenameSize);
			SAFE_WRITE(fd,&new_nodeid,0,sizeof(int),sizeof(int));
			// Increase datablock's counter
			dataCounter++;
			CALL(lseek(fd,(parent_data.datablocks[i])*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(fd,&dataCounter,0,sizeof(int),sizeof(int));
			// If this is the first entity in datablock
			break;
		}
	}

	parent_mds->size += sB.filenameSize + sizeof(int);

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
	metadata->size = 2*(sB.filenameSize + sizeof(int));
	metadata->type = Directory;
	metadata->parent_nodeid = parent_nodeid;
	curr_time = time(NULL);
	metadata->creation_time = curr_time;
	metadata->access_time = curr_time;
	metadata->modification_time = curr_time;
	metadata->datablocksCounter = 1;

	// Create directory's data (one block for now)
	offset += sizeof(MDS);
	data.datablocks = (int*) (inodeTable + offset);
	data.datablocks[0] = getEmptyBlock();
	CALL(lseek(fd,data.datablocks[0]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	// Write entities' counter for current datablock
	int	entityCounter = 2;
	SAFE_WRITE(fd,&entityCounter,0,sizeof(int),sizeof(int));
	// Write entity (filename + nodeid) for relative path "."
	CALL(write(fd,".",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&(metadata->nodeid),0,sizeof(int),sizeof(int));
	// Write entity for relative path ".."
	CALL(write(fd,"..",3),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-3,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&(metadata->parent_nodeid),0,sizeof(int),sizeof(int));
	// Directory's rest datablocks will be empty for now
	for(i=1; i<sB.maxFileDatablockNum; i++)
		data.datablocks[i] = -1;

	return new_nodeid;
}

int cfs_touch(int fd,char *filename,touch_mode mode)
{
	int	ignore = 0;

	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	if(mode == CRE)
	{
		int		parent_nodeid, new_nodeid;
		int		i, offset;
		char		new_name[sB.filenameSize+1];
		char	parent_path[strlen(filename)+1];
		bool		busy;
		time_t		curr_time;
		MDS		*parent_mds, *metadata;
		Datastream	parent_data, data;

		// Initialize new_name
		for(int i=0; i<=sB.filenameSize; i++)
			new_name[i] = '\0';

		// Get parent's nodeid and new file's name
		parent_nodeid = get_parent(fd,filename,new_name);
		if(parent_nodeid == -1)
			return -1;
	
		// Go to parent's metadata
		parent_mds = (MDS*) (inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		// Check if parent is a directory
		if(parent_mds->type != Directory)
		{
			printf("Input error, %s is not a directory.\n",inodeTable+parent_nodeid*inodeSize+sizeof(bool));
			return -1;
		}

		// Check if file exists
		if(traverse_cfs(fd,new_name,parent_nodeid) != -1)
		{
			printf("Input error, file %s already exists.\n",filename);
			return -1;
		}

		// Go to parent's data
		parent_data.datablocks = (int*)(inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

		int	dataCounter, move, numofEntities;
		if (strlen(filename)-strlen(new_name) > 0) {
			strncpy(parent_path, filename, strlen(filename)-strlen(new_name));
			parent_path[strlen(filename)-strlen(new_name)] = '\0';
		} else {
			strcpy(parent_path, "./");
		}
		numofEntities = getDirEntities(fd,parent_path,NULL);

		// Update parent directory's data
		for(i=0; i<sB.maxFileDatablockNum; i++)
		{
			// If current datablock is empty
			if(parent_data.datablocks[i] == -1)
			{
				parent_data.datablocks[i] = getEmptyBlock();
				// Go to datablock
				move = parent_data.datablocks[i]*sB.blockSize + sizeof(int);
				CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				dataCounter = 0;
				parent_mds->datablocksCounter++;
			}
			else
			{
				// Go to datablock
				move = parent_data.datablocks[i]*sB.blockSize;
				CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(fd,&dataCounter,0,sizeof(int),sizeof(int));
			}

			// If parent directory has no datablocks available
			if(parent_mds->datablocksCounter == sB.maxDirDatablockNum && numofEntities == sB.maxEntitiesPerBlock*sB.maxDirDatablockNum)
			{
				printf("Not enough space in parent directory to create file %s.\n",filename);
				return -1;
			}
	
			// If there is space for another entity
			if(dataCounter < sB.maxEntitiesPerBlock)
			{
				// Find an empty space in inodeTable
				new_nodeid = getTableSpace();

				// Go after the last entity in the datablockrealloc
				CALL(lseek(fd,dataCounter*(sB.filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(fd,new_name,0,sizeof(char),sB.filenameSize);
				SAFE_WRITE(fd,&new_nodeid,0,sizeof(int),sizeof(int));
				// Increase datablock's counter
				dataCounter++;
				CALL(lseek(fd,(parent_data.datablocks[i])*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(fd,&dataCounter,0,sizeof(int),sizeof(int));
				break;
			}
		}

		parent_mds->size += sB.filenameSize + sizeof(int);

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
		metadata->datablocksCounter = 0;

		offset += sizeof(MDS);
		data.datablocks = (int*) (inodeTable + offset);
		for(i=0; i<sB.maxFileDatablockNum; i++)
			data.datablocks[i] = -1;

		return new_nodeid;
	}
	else
	{
		int	start, nodeid;
		char	temp[sB.filenameSize];
		time_t	curr_time;
		MDS	*current_mds, *metadata;

		// If path starts with "/"
		if(!strncmp(filename,"/",1))
			// Traversing cfs will start from the root (parent_nodeid = 0)
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

		nodeid = traverse_cfs(fd,filename,start);
		if(nodeid == -1)
		{
			printf("Path %s could not be found in cfs.\n",filename);
			return -1;
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

		return nodeid;
	}
}

int cfs_ln(int fileDesc,char* source_path,char* destination)
{
	int 	 ignore;

	char 	source_name[sB.filenameSize];
	int 	 source_nodeid;
	MDS		*source_mds;

	int 	 directory_nodeid;
	MDS		*directory_mds;
	int		*directory_data;

	char	 destination_path[strlen(destination) + sB.filenameSize + 2];
	char	 destination_name[sB.filenameSize];
	int		 destination_nodeid;
	MDS		*destination_mds;

	// Check if source exists.
	get_parent(fileDesc, source_path, source_name);
	source_nodeid = traverse_cfs(fileDesc, source_path, getPathStartId(source_path));
	if (source_nodeid == -1) {
		printf("ln: cannot stat '%s': No such file or directory\n",source_path);
		return -1;
	}
	// Check if source is file.
	// source_name = inodeTable + source_nodeid*inodeSize + sizeof(bool);
	source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	if (source_mds->type == Directory) {
		printf("ln: cannot create link to directory\n");
		return -1;
	}
	// Check if destination exists.
	directory_nodeid = get_parent(fileDesc, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("ln: cannot create directory '%s': No such file or directory\n", destination);
		return -1;
	}

	destination_nodeid = traverse_cfs(fileDesc,destination_name,directory_nodeid);
	strcpy(destination_path, destination);
	if (destination_nodeid != -1) {
		destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
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
	directory_mds = (MDS*) (inodeTable + directory_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	// Go to directory's data.
	directory_data = (int*)(inodeTable + directory_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	int	dataCounter, move, numofEntities = 0;

	// Update directory's data.
	for(int i=0; i<sB.maxFileDatablockNum; i++)
	{
		// Check if current datablock is empty.
		if(directory_data[i] == -1)
		{
			directory_data[i] = getEmptyBlock();
			// Go to datablock
			move = directory_data[i]*sB.blockSize + sizeof(int);
			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			dataCounter = 0;
			directory_mds->datablocksCounter++;
		}
		else
		{
			// Go to datablock
			move = directory_data[i]*sB.blockSize;
			CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			numofEntities += dataCounter;
		}
		// If directory has no datablocks available
		if(directory_mds->datablocksCounter == sB.maxDirDatablockNum && numofEntities == sB.maxEntitiesPerBlock*sB.maxDirDatablockNum)
		{
			printf("ln: not enough space in directory to create link '%s' to '%s'.\n",source_name, destination_path);
			return -1;
		}
		// If there is space for another entity
		if(dataCounter < sB.maxEntitiesPerBlock)
		{
			source_mds->linkCounter++;

			// Go after the last entity in the datablock
			CALL(lseek(fileDesc,dataCounter*(sB.filenameSize+sizeof(int)),SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(fileDesc,destination_name,0,sizeof(char),sB.filenameSize);
			SAFE_WRITE(fileDesc,&source_nodeid,0,sizeof(int),sizeof(int));
			// Increase datablock's counter
			dataCounter++;
			CALL(lseek(fileDesc,(directory_data[i])*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_WRITE(fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			break;
		}
	}

	directory_mds->size += sB.filenameSize + sizeof(int);
	return source_nodeid;
}

bool cfs_pwd() 
{
	// Takes the current nodeid from global variable cfs_current_nodeid,
	MDS 			*curr_dir_MDS = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	int 	 		 curr_dir_id = cfs_current_nodeid;
	// Traverse the nodes until the root. For each one save its id in a list path_nodeIds,
	List 			*path_nodeIds = NULL;
	while (curr_dir_id != 0) {
		addNode(&path_nodeIds, curr_dir_id);
		curr_dir_id = curr_dir_MDS->parent_nodeid;
		curr_dir_MDS = (MDS *) (inodeTable + curr_dir_id*inodeSize + sizeof(bool));
	}
	// addNode(path_nodeIds, curr_dir_id);

	// For debuging only,
	// printf("<");
	// printList(path_nodeIds);
	// printf(">");

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
			curr_dir_MDS = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			fileName = (inodeTable + curr_dir_id*inodeSize + sizeof(bool));;
			printf("/%s", fileName);
		}
	}
	return true;
}

bool cfs_cd(int fileDesc, char *path) 
{
	int 	 path_nodeId = -1;
	MDS 	*path_MDS = NULL;
	int		 start;

	// Check if the path has value in it.
	if (path != NULL) {
		char	initial_path[strlen(path)+1];

		// If path starts with "/"
		if(path[0] == '/')
		{
			// Traversing cfs will start from the root (nodeid = 0)
			path_nodeId = 0;
		}
		else
		{
			// Get first entity in path
			strcpy(initial_path,path);
			char *start_dir = strtok(initial_path,"/");
			// If path starts from current directory's parent
			if(strcmp(start_dir,"..") == 0)
			{
				path_MDS = (MDS *) (inodeTable + cfs_current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				start = path_MDS->parent_nodeid;
			}
			else	//if(!strcmp(split,".") || strcmp(split,"/"))
			{
				// If path starts from current directory
				start = cfs_current_nodeid;
			}

		}
		path_nodeId = traverse_cfs(fileDesc, path, start);
		if(path_nodeId == -1)
		{
			printf("Path %s could not be found in cfs.\n",path);
			return false;
		}
	} else {
		cfs_current_nodeid = 0;
		return true;
	}
	// Check if the path has valid value.
	if(path_nodeId == -1) {
		printf("Path is not valid.\n");
		return false;
	} else {
		path_MDS = (MDS *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize);
	}
	// Check if the last entity is directory.
	if (path_MDS->type != Directory) {
		printf("Input error, '%s' is not a valid path.\n",path);
		return false;
	} else {
		cfs_current_nodeid = path_nodeId;
	}

	return true;
}

bool cfs_ls(int fileDesc, bool *ls_modes, char *path) 
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
	int		 		 start;

	// Search the nodeid of the path,
	if (path == NULL) {
		path_nodeId = cfs_current_nodeid;
		path_MDS = (MDS *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize);
	} else {
		char	initial_path[strlen(path)+1];

		// If path starts with "/"
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
			path_nodeId = cfs_current_nodeid;
			// If path starts from current directory's parent
			if(strcmp(start_dir,"..") == 0)
			{
				path_MDS = (MDS *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize);
				start = path_MDS->parent_nodeid;
			}
			else	//if(!strcmp(split,".") || strcmp(split,"/"))
			{
				// If path starts from current directory
				start = cfs_current_nodeid;
			}
		}
		path_nodeId = traverse_cfs(fileDesc, path, start);

		// Check if the path has valid value.
		if(path_nodeId != -1) {
			path_MDS = (MDS *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize);
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

	// Get the list with the nodeids of the path's content,
	path_data = (int *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	int dataStreamIndex=0;
	int pairCounter;
	current_filename = (char*) malloc(sB.filenameSize);
	for (int i=0; i < path_MDS->datablocksCounter; i++) {
		// Ignore the cells of the table which have -1 var
		while (path_data[dataStreamIndex] == -1) {
			dataStreamIndex++;
		}
		CALL(lseek(fileDesc,path_data[dataStreamIndex]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
		SAFE_READ(fileDesc,&pairCounter,0,sizeof(int),sizeof(int));
		for (int j=0; j < pairCounter; j++) {
			CALL(lseek(fileDesc,path_data[dataStreamIndex]*sB.blockSize + sizeof(int) + j*(sB.filenameSize+sizeof(int)),SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(fileDesc,current_filename,0,sizeof(char),sB.filenameSize);
			SAFE_READ(fileDesc,&current_nodeid,0,sizeof(int),sizeof(int));
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if(strcmp(current_filename,".") && strcmp(current_filename,"..") && current_MDS->type == Directory)
				addNode(&path_content, current_nodeid);
			if ((ls_modes[LS_D] == true && current_MDS->type == Directory) || (ls_modes[LS_H] == true && current_MDS->type == Link) || (ls_modes[LS_D] == false && ls_modes[LS_H] == false))
				add_stringNode(&path_content_names, current_filename);
		}
		dataStreamIndex++;
	}
	free(current_filename);
	// Print them, with the right option,
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
	while (path_content_names != NULL) {
		if (ls_modes[LS_U] == true) {
			current_filename = pop_last_string(&path_content_names);
			// current_filename = pop_string(&path_content_names);
		} else {
			current_filename = pop_minimum_string(&path_content_names);
		}
		if ((ls_modes[LS_A] == false && current_filename[0] != '.') || ls_modes[LS_A] == true) {
			current_nodeid = traverse_cfs(fileDesc, current_filename, path_nodeId);
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
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
	if (ls_modes[LS_R] == true) {
		char *temp = (char*) malloc(strlen(dirName)+1);
		int tempSize = strlen(temp);
		int dirPathSize;
		while (path_content != NULL) {
			strcpy(temp, dirName);
			current_nodeid = pop_last_Node(&path_content);
			current_filename = inodeTable + current_nodeid*inodeSize + sizeof(bool);
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			// if (current_MDS->type == Directory && current_MDS->nodeid != path_nodeId && current_MDS->nodeid != path_MDS->parent_nodeid) {
				dirPathSize = strlen(temp) + strlen(current_filename) + 2;
				if (tempSize < dirPathSize)
					temp = realloc(temp, dirPathSize);
				strcat(temp, "/");
				strcat(temp, current_filename);
				
				printf("\n");
				cfs_ls(fileDesc, ls_modes, temp);
			// }
		}
		free(temp);
	}
	free(dirName);
	return true;
}

bool cfs_cp(int fileDesc, bool *cp_modes, string_List *sourceList, char *destination)
{
	int 		 sourcesCount = getLength(sourceList);
	char		 line[100];
	char		*answer;
	string_List *directory_inodeList;

	char 		*source_path;
	char 		 source_name[sB.filenameSize];
	int 		 source_nodeid;
	MDS 		*source_mds;

	int 		 directory_nodeid;

	char		 destination_path[strlen(destination) + sB.filenameSize + 2];
	char 		 destination_name[sB.filenameSize];
	int			 destination_nodeid;
	MDS 		*destination_mds;

	//------------------------------------------------------------------------------------
	// Get destination's directory inode
	directory_nodeid = get_parent(fileDesc, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("cp: cannot create directory '%s': No such file or directory\n", destination);
		return false;
	}
	//------------------------------------------------------------------------------------
	// Copy each source to destination.
	while (sourceList != NULL) {
		source_path = pop_last_string(&sourceList);
		// Get source inode.
		get_parent(fileDesc, source_path, source_name);
		source_nodeid = traverse_cfs(fileDesc, source_path, getPathStartId(source_path));
		// Check if source_path has valid value.
		if(source_nodeid == -1) {
			printf("cp: cannot stat '%s': No such file or directory\n",source_path);
			free(source_path);
			continue;
		}
		// source_name = inodeTable + source_nodeid*inodeSize + sizeof(bool);
		source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		if (source_mds->type == Directory && cp_modes[CP_R] == false && cp_modes[CP_RR] == false) {
			printf("cp: -r not specified; omitting directory '%s'\n", source_path);
			free(source_path);
			continue;
		}
		//--------------------------------------------------------------------------------
		// Get destination inode.
		destination_nodeid = traverse_cfs(fileDesc,destination_name,directory_nodeid);
		strcpy(destination_path, destination);
		if (destination_nodeid == -1) {
			if (sourcesCount != 1) {
				printf("cp: target '%s' is not a directory\n", destination);
				free(source_path);
			} else if (source_mds->type == Directory) {
				if (cp_modes[CP_RR] == true) {
					cfs_mkdir(fileDesc, destination_path);
				} else {	// (cp_modes[CP_R] == true)
					directory_inodeList = NULL;
					getDirEntities(fileDesc, source_path, &directory_inodeList);
					cfs_mkdir(fileDesc, destination_path);
					cfs_cp(fileDesc, cp_modes, directory_inodeList, destination_path);
				}
			} else {
				cfs_touch(fileDesc, destination_path, CRE);
				destination_nodeid = traverse_cfs(fileDesc, destination_path, getPathStartId(destination_path));
				replaceEntity(fileDesc, source_nodeid, destination_nodeid);
			}
		} else {	// destination_nodeid != -1
			// If the file exists check the its type.
			destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if (destination_mds->type == Directory) {
				strcat(destination_path, "/");
				strcat(destination_path, source_name);
				destination_nodeid = traverse_cfs(fileDesc,source_name,destination_nodeid);
				if (destination_nodeid == -1) {
					if (source_mds->type == Directory) {
						if (cp_modes[CP_RR] == true) {
							cfs_mkdir(fileDesc, destination_path);
						} else {	// (cp_modes[CP_R] == true)
							directory_inodeList = NULL;
							getDirEntities(fileDesc, source_path, &directory_inodeList);
							cfs_mkdir(fileDesc, destination_path);
							cfs_cp(fileDesc, cp_modes, directory_inodeList, destination_path);
						}
					} else {
						cfs_touch(fileDesc, destination_path, CRE);
						destination_nodeid = traverse_cfs(fileDesc, destination_path, getPathStartId(destination_path));
						replaceEntity(fileDesc, source_nodeid, destination_nodeid);
					}
				} else {	// destination_nodeid != -1.
					// If destination/source_name exists check if the type of source and destination are matching.
					destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
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
						getDirEntities(fileDesc, source_path, &directory_inodeList);
						cfs_cp(fileDesc, cp_modes, directory_inodeList, destination_path);
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
						cfs_touch(fileDesc, destination_path, MOD);
						replaceEntity(fileDesc, source_nodeid, destination_nodeid);
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
					cfs_touch(fileDesc, destination_path, MOD);
					replaceEntity(fileDesc, source_nodeid, destination_nodeid);
					destination_mds->linkCounter = 1;
				}
			}
		}
		free(source_path);
	}
	return true;
}

bool cfs_mv(int fileDesc, bool *mv_modes, string_List *sourceList, char *destination)
{
	int 		 sourcesCount = getLength(sourceList);
	char		 line[100];
	char		*answer;
	string_List *directory_inodeList;
	bool		rm_modes[rm_mode_Num];

	char 		*source_path;
	char 		 source_name[sB.filenameSize];
	int 		 source_nodeid;
	MDS 		*source_mds;

	int 		 directory_nodeid;

	char		 destination_path[strlen(destination) + sB.filenameSize + 2];
	char 		 destination_name[sB.filenameSize];
	int			 destination_nodeid;
	MDS 		*destination_mds;

	for (int i=0; i<rm_mode_Num; i++)
		rm_modes[i] = false;
	//------------------------------------------------------------------------------------
	// Get destination's directory inode
	directory_nodeid = get_parent(fileDesc, destination, destination_name);
	if (directory_nodeid == -1) {
		printf("mv: cannot create directory '%s': No such file or directory\n", destination);
		return false;
	}
	//------------------------------------------------------------------------------------
	// Copy each source to destination.
	while (sourceList != NULL) {
		source_path = pop_last_string(&sourceList);
		// Get source inode.
		get_parent(fileDesc, source_path, source_name);
		source_nodeid = traverse_cfs(fileDesc, source_path, getPathStartId(source_path));
		// Check if source_path has valid value.
		if(source_nodeid == -1) {
			printf("mv: cannot stat '%s': No such file or directory\n",source_path);
			free(source_path);
			continue;
		}
		// source_name = inodeTable + source_nodeid*inodeSize + sizeof(bool);
		source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		//--------------------------------------------------------------------------------
		// Get destination inode.
		destination_nodeid = traverse_cfs(fileDesc,destination_name,directory_nodeid);
		strcpy(destination_path, destination);
		if (destination_nodeid == -1) {
			// If the destination does not exists, it has to be imported only one source.
			// Create a new inode at the destination and copy the source in it.
			if (sourcesCount != 1) {
				printf("mv: target '%s' is not a directory\n", destination);
			} else if (source_mds->type == Directory){
				directory_inodeList = NULL;
				getDirEntities(fileDesc, source_path, &directory_inodeList);
				cfs_mkdir(fileDesc, destination_path);
				cfs_mv(fileDesc, mv_modes, directory_inodeList, destination_path);
			} else {	// source_mds->type == File.
				cfs_touch(fileDesc, destination_path, CRE);
				destination_nodeid = traverse_cfs(fileDesc, destination_path, getPathStartId(destination_path));
				replaceEntity(fileDesc, source_nodeid, destination_nodeid);
			}
		} else {	// destination_nodeid != -1.
			// If the destination exists check its type.
			destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if (destination_mds->type == Directory) {
				// If destination is a directory check if exists the destination/source_name.
				strcat(destination_path, "/");
				strcat(destination_path, source_name);
				destination_nodeid = traverse_cfs(fileDesc,source_name,destination_nodeid);
				if (destination_nodeid == -1) {
					// If destination/source_name does not exists create a new directory or file with name <source_name>.
					if(source_mds->type == Directory) {
						directory_inodeList = NULL;
						getDirEntities(fileDesc, source_path, &directory_inodeList);
						cfs_mkdir(fileDesc, destination_path);
						cfs_mv(fileDesc, mv_modes, directory_inodeList, destination_path);
					} else {
						cfs_touch(fileDesc, destination_path, CRE);
						destination_nodeid = traverse_cfs(fileDesc, destination_path, getPathStartId(destination_path));
						replaceEntity(fileDesc, source_nodeid, destination_nodeid);
					}
				} else {	// destination_nodeid != -1.
					// If destination/source_name exists check if the type of source and destination are matching.
					destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
					if (source_mds->type == Directory && destination_mds->type == Directory) {
						if (mv_modes[MV_I] == true) {
							printf("mv: You want to overide '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Copy denied.\n");
								free(source_path);
								cfs_rm(fileDesc, rm_modes, source_path);
								continue;
							}
						}
						directory_inodeList = NULL;
						getDirEntities(fileDesc, source_path, &directory_inodeList);
						cfs_mv(fileDesc, mv_modes, directory_inodeList, destination_path);
					} else if (source_mds->type == File && destination_mds->type == File) {
						if (mv_modes[MV_I] == true) {
							printf("mv: You want to overwrite '%s' to '%s'? (Press y|Y for yes) ", source_path, destination_path);
							fgets(line, 100, stdin);
							if (line != NULL){
								answer = strtok(line, " \n\t");
							}
							if (answer == NULL || (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0)) {
								printf("Copy denied.\n");
								cfs_rm(fileDesc, rm_modes, source_path);
								free(source_path);
								continue;
							}
						}
						cfs_touch(fileDesc, destination_path, MOD);
						replaceEntity(fileDesc, source_nodeid, destination_nodeid);
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
							printf("Copy denied.\n");
							cfs_rm(fileDesc, rm_modes, source_path);
							free(source_path);
							continue;
						}
					}
					cfs_touch(fileDesc, destination_path, MOD);
					replaceEntity(fileDesc, source_nodeid, destination_nodeid);
					destination_mds->linkCounter = 1;
				}
			}
		}

		cfs_rm(fileDesc, rm_modes, source_path);
		free(source_path);
	}

	return true;
}

bool cfs_rm(int fileDesc, bool *modes, char *dirname)
{
	int		move, ignore = 0;
	int		start, nodeid;
	int		dataCounter, checked = 0;
	MDS		*metadata;
	Datastream	data;

	// Find the nodeid of the first entity in path 'dirname'
	start = getPathStartId(dirname);
	// Get the nodeid of the entity the destination path leads to
	nodeid = traverse_cfs(fileDesc,dirname,start);
	// If destination parth is invalid
	if(nodeid == -1)
	{
		printf("Path %s could not be found in cfs.\n",dirname);
		return false;
	}

	metadata = (MDS*) (inodeTable + nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	data.datablocks = (int*) (inodeTable + nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
	// If it is not a directory
	if(metadata->type != Directory)
	{
		printf("Input error, %s is not a directory.\n",dirname);
		return false;
	}

	char		content_name[sB.filenameSize];
	int		content_nodeid, newCounter;
	MDS		*content_mds;
	Datastream	content_data;
	// For directory's each datablock
	for(int i=0; i<sB.maxFileDatablockNum; i++)
	{
		// If datablock has contents
		if(data.datablocks[i] != -1)
		{	// Get number of entities in current datablock
			CALL(lseek(fileDesc,data.datablocks[i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
			SAFE_READ(fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
			// Number of entities in datablock after remove is completed
			newCounter = 0;
			// For every entity in datablock
			for(int j=0; j<dataCounter; j++)
			{
				move = data.datablocks[i]*sB.blockSize + sizeof(int) + j*(sB.filenameSize + sizeof(int));
				CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				// Get current entity's name
				SAFE_READ(fileDesc,content_name,0,sizeof(char),sB.filenameSize);
				// If it is not the current or parent entity
				if(strcmp(content_name,".") && strcmp(content_name,".."))
				{	// Get entity's nodeid
					SAFE_READ(fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));

					// If '-i' was given as an argument
					if(modes[RM_I] == true)
					{	// Ask user for confirmation
						char	answer[4];
						printf("Are you sure you want to delete item %s ?(yes/no):\n",content_name);
						scanf("%s",answer);
						// If user said no, go to the next entity
						if(!strcmp(answer,"no"))
						{
							// If there is a hole inside datablock, fill it
							if(j > newCounter)
							{
								move = data.datablocks[i]*sB.blockSize + sizeof(int) + newCounter*(sB.filenameSize+sizeof(int));
								CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
								SAFE_WRITE(fileDesc,content_name,0,sizeof(char),sB.filenameSize);
								SAFE_WRITE(fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));
							}
							// Increase newCounter and go to the next entity
							newCounter++;
							continue;
						}
					}

					content_mds = (MDS*) (inodeTable + content_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
					content_data.datablocks = (int*) (inodeTable + content_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));
					// If it is a file, it will be removed
					if(content_mds->type == File)
					{
						content_mds->linkCounter--;
						if(!content_mds->linkCounter)
						{
							// Push allocated datablocks in holeList
							for(int k=0; k<sB.maxFileDatablockNum; k++)
								if(content_data.datablocks[k] != -1)
								{
									addNode(&holes,content_data.datablocks[k]);
									sB.ListSize++;
								}
						}
					}
					else	// If it is a directory
					{	// If it has contnents
						if(content_mds->datablocksCounter != 0)
						{	// If '-r' was given as an argument, it will be removed
							if(modes[RM_R] == true)
							{
								char	recursion_name[strlen(dirname)+sB.filenameSize+2];
								strcpy(recursion_name,dirname);
								strcat(recursion_name,"/");
								strcat(recursion_name,content_name);
								cfs_rm(fileDesc,modes,recursion_name);
								// Remove directory's first datablock (with the current and parent entities)
								addNode(&holes,content_data.datablocks[0]);
								sB.ListSize++;
							}
							// Else, it will not be removed. If there is a hole inside datablock, fill it
							else if(j > newCounter)
							{
								move = data.datablocks[i]*sB.blockSize + sizeof(int) + newCounter*(sB.filenameSize+sizeof(int));
								CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
								SAFE_WRITE(fileDesc,content_name,0,sizeof(char),sB.filenameSize);
								SAFE_WRITE(fileDesc,&content_nodeid,0,sizeof(int),sizeof(int));
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
					*(bool*) (inodeTable + content_nodeid*inodeSize) = false;
					metadata->size -= sB.filenameSize + sizeof(int);
					sB.nodeidCounter--;
					// If it is the last in inodeTable do not keep the hole (inner holes should be maintained)
					if(content_nodeid == (sB.iTableCounter-1))
						sB.iTableCounter--;
				}
				// If it is the current or parent entity
				else
					newCounter++;
			}
			// If datablock has still contents (maybe only current and parent entity), update counter
			if(newCounter > 0)
			{
				CALL(lseek(fileDesc,data.datablocks[i]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_WRITE(fileDesc,&newCounter,0,sizeof(int),sizeof(int));
			}
			// Else, add it to holeList and remove it from directory's data
			else
			{
				addNode(&holes,data.datablocks[i]);
				sB.ListSize++;
				data.datablocks[i] = -1;
				metadata->datablocksCounter--;
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

bool cfs_cat(int fileDesc, string_List *sourceList, char *outputPath)
{
	int		start;
	unsigned int	total_size = 0, output_size;
	int		source_nodeid, output_nodeid;
	MDS		*source_mds, *output_mds;

	// Find the nodeid of the first entity in 'outputPath'
	start = getPathStartId(outputPath);
	// Get the nodeid of the entity the output path leads to
	output_nodeid = traverse_cfs(fileDesc,outputPath,start);
	// If output path was invalid
	if(output_nodeid == -1)
	{
		printf("Path %s could not be found in cfs.\n",outputPath);
		return false;
	}
	// Get output file's size
	output_mds = (MDS*) (inodeTable + output_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	if(output_mds->type != File)
	{
		printf("Input error, output path %s is not a file.\n",inodeTable+output_nodeid*inodeSize+sizeof(bool));
		return false;
	}
	output_size = output_mds->size;

	int	listSize, i;
	char	*source_name = NULL;
	// Get number of files in sourceList
	listSize = getLength(sourceList);
	// For every source file
	for(i=1; i<=listSize; i++)
	{
		source_name = get_stringNode(&sourceList,i);
		if(source_name != NULL)
		{
			start = getPathStartId(source_name);
			source_nodeid = traverse_cfs(fileDesc,source_name,start);
			if(source_nodeid == -1)
			{
				printf("Path %s could not be found in cfs.\n",source_name);
				return false;
			}
			source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if(source_mds->type != File)
			{
				printf("Input error, source path %s is not a file.\n",inodeTable+source_nodeid*inodeSize+sizeof(bool));
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
		printf("Input error, problem with list of source files.\n");
		return false;
	}

	if(total_size > (sB.maxFileSize - output_size))
	{
		printf("Not enough space in output file to cat all source files.\n");
		return false;
	}

	for(int i=0; i<listSize; i++)
	{
		source_name = pop_last_string(&sourceList);
		start = getPathStartId(source_name);
		source_nodeid = traverse_cfs(fileDesc,source_name,start);
		if(source_nodeid == -1)
		{
			printf("Path %s could not be found in cfs.\n",source_name);
			free(source_name);
			return false;
		}
		source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		append_file(fileDesc,source_nodeid,output_nodeid);
		free(source_name);
	}

	return true;
}

bool cfs_import(int fileDesc,string_List *sourceList,char *destPath)
{
	int		ignore = 0;
	int		source_nodeid, dest_nodeid, dest_entities;
	char		*initial;
	char		new_path[strlen(destPath)+sB.filenameSize+2];
	MDS		*source_mds, *dest_mds;
	Datastream	data;

	// Get the nodeid of the entity the destination path leads to
	dest_nodeid = traverse_cfs(fileDesc,destPath,getPathStartId(destPath));
	// If destination path was invalid
	if(dest_nodeid == -1)
	{
		printf("Path %s could not be found in cfs.\n",destPath);
		return false;
	}
	// Get destination directory's metadata
	dest_mds = (MDS*) (inodeTable + dest_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	if(dest_mds->type != Directory)
	{
		printf("Input error, destination path %s is not a directory.\n",inodeTable+dest_nodeid*inodeSize+sizeof(bool));
		return false;
	}
	dest_entities = getDirEntities(fileDesc,destPath,NULL);

	int	listSize, i;
	char	*source_name = NULL;
	// Get number of entities in sourceList
	listSize = getLength(sourceList);
	// If source entities require too much space
	if(listSize > (sB.maxFilesPerDir - dest_entities))
	{
		printf("Error, not enough space in destination directory.\n");
		return false;
	}

	int		move;
	char		fileBuffer[sB.blockSize];
	char		*split, *temp;
	struct stat	entity;
	// For every source file
	for(i=0; i<listSize; i++)
	{
		source_name = pop_last_string(&sourceList);
		if(source_name != NULL)
		{
			// If source entity does not exist
			if(access(source_name,F_OK) == -1)
			{
				printf("Input error, source entity %s does not exist.\n",source_name);
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
				char		content_path[strlen(source_name)+sB.filenameSize+2];

				if((dir = opendir(source_name)) == NULL)
				{
					perror("Error opening source directory: ");
					free(source_name);
					return false;
				}

				// Create a new directory in cfs (under the appropriate parent directory)
				source_nodeid = cfs_mkdir(fileDesc,new_path);
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
						cfs_import(fileDesc,contentList,new_path);
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
				source_nodeid = cfs_touch(fileDesc,new_path,CRE);
				if(source_nodeid == -1)
				{
					free(source_name);
					return false;
				}
				source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				data.datablocks = (int*) (inodeTable + source_nodeid*inodeSize +sizeof(bool) + sB.filenameSize + sizeof(MDS));

				// Get file's size
				source_end = entity.st_size;
				size_to_read = (sB.blockSize > source_end) ? source_end : sB.blockSize;
				// Go to the beginning and read file's contents
				CALL(lseek(source_fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(source_fd,fileBuffer,0,sizeof(char),size_to_read);

				// Fill file's data
				for(int j=0; j<(sB.maxFileDatablockNum); j++)
				{
					data.datablocks[j] = getEmptyBlock();
					// File's datablocks are increased by one
					source_mds->datablocksCounter++;
					move = data.datablocks[j]*sB.blockSize;
					CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					// Write source file's contents
					SAFE_WRITE(fileDesc,fileBuffer,0,sizeof(char),size_to_read);

					// Read source file's next contents
					// Get current location in source directory
					CALL(lseek(source_fd,0,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,current);
					// If there are still info to read
					if(current < source_end)
					{
						size_to_read = (sB.blockSize > (source_end-current)) ? (source_end-current) : (sB.blockSize);
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
		printf("Input error, problem with list of source files.\n");
		return false;
	}

	return true;
}

int cfs_create(char *filename,int bSize,int nameSize,unsigned int maxFSize,int maxDirFileNum)
{
	int		fd = 0, ignore = 0;
	time_t		current_time;
	MDS		metadata;
	Datastream	data;

	holes = NULL;

	CALL(creat(filename, S_IRWXU | S_IXGRP),-1,"Error creating file for cfs: ",1,fd);

	// Create superblock
	// Block size has to be a multiple of 512 bytes
	if(bSize != -1 && (bSize % BLOCK_SIZE))
	{
		printf("Input error, block size is too small. Please try again.\n");
		return -1;
	}
	sB.blockSize = (bSize != -1) ? bSize : BLOCK_SIZE;
	// Filename size has a ceiling (252 bytes)
	if(nameSize != -1 && nameSize > MAX_FILENAME_SIZE)
	{
		printf("Input error, filename size is too long. Please try again.\n");
		return -1;
	}
	sB.filenameSize = (nameSize != -1) ? nameSize : FILENAME_SIZE;
	sB.maxFileSize = (maxFSize != -1) ? maxFSize : MAX_FILE_SIZE;
	sB.maxFilesPerDir = (maxDirFileNum != -1) ? maxDirFileNum : MAX_DIRECTORY_FILE_NUMBER;
	///	sB.metadataBlocksNum = (sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) / sB.blockSize;
	///	if(((sB.filenameSize + sizeof(MDS) + (sB.maxDatablockNum)*sizeof(unsigned int)) % sB.blockSize) > 0)
	///        	sB.metadataBlocksNum++;
	// Every datablock of a directory contains some pairs of (filename+nodeid) for the directory's contents and a counter of those pairs
	sB.maxEntitiesPerBlock = (sB.blockSize - sizeof(int)) / (sB.filenameSize + sizeof(int));
	sB.maxFileDatablockNum = (sB.maxFileSize) / (sB.blockSize);
	//	if((sB.blockSize - sizeof(int)) % (sB.filenameSize + sizeof(int)))
	if(sB.maxFileSize % sB.blockSize)
		sB.maxFileDatablockNum++;
	sB.maxDirDatablockNum = (sB.maxFilesPerDir + 2) / sB.maxEntitiesPerBlock;
	if((sB.maxFilesPerDir + 2) % sB.maxEntitiesPerBlock)
		sB.maxDirDatablockNum++;

	// Assume block-counting is zero based	
	sB.blockCounter = 0;
	// Only root in cfs for now
	sB.nodeidCounter = 1;
	sB.ListSize = 0;
	// Assume that root's metadata can be stored in one block for now (sB.filenameSize+sizeof(MDS)+sizeof(int))
	sB.iTableBlocksNum = 1;
	sB.iTableCounter = sB.nodeidCounter;
	// Get superblock's overflow block (contains the inodeTable and the holeList)
	sB.nextSuperBlock = getEmptyBlock();
	CALL(lseek(fd,sB.nextSuperBlock*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	int	overflow_block = -1;
	SAFE_WRITE(fd,&overflow_block,0,sizeof(int),sizeof(int));

	int	blocknum;
	// Get block for root's meatdata (one block for now)
	blocknum = getEmptyBlock();
	sB.root = blocknum;
	// Write blocknum in iTable map
	SAFE_WRITE(fd,&blocknum,0,sizeof(int),sizeof(int));

	// Write superBlock	
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&sB,0,sizeof(superBlock),sizeof(superBlock));

	// Go to block with root's metadata (one block for now)
	CALL(lseek(fd,blocknum*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);

	// Write root's name (cannot be larger than a block)
	CALL(write(fd,"/",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);

	metadata.nodeid = 0;
	// Root's data contain the relative paths (. & ..)
	metadata.size = 2*(sB.filenameSize + sizeof(int));
	metadata.type = Directory;
	metadata.parent_nodeid = 0;
	current_time = time(NULL);
	metadata.creation_time = current_time;
	metadata.access_time = current_time;
	metadata.modification_time = current_time;
	// Entities for . and .. can be stored in one block
	metadata.datablocksCounter = 1;

	// Write root's metadata
	SAFE_WRITE(fd,&metadata,0,sizeof(MDS),sizeof(MDS));

	// Create root's data blocks (one block for now)
	data.datablocks = (int*)malloc(sizeof(int));
	data.datablocks[0] = getEmptyBlock();
	CALL(lseek(fd,data.datablocks[0]*sB.blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	// Write entities' counter for current datablock
	int	entityCounter = 2;
	SAFE_WRITE(fd,&entityCounter,0,sizeof(int),sizeof(int));
	// Write entity (filename + nodeid) for relative path "."
	CALL(write(fd,".",2),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-2,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&(metadata.nodeid),0,sizeof(int),sizeof(int));
	// Write entity for relative path ".."
	CALL(write(fd,"..",3),-1,"Error writing in cfs file: ",3,ignore);
	// Leave filenameSize space free
	CALL(lseek(fd,sB.filenameSize-3,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&(metadata.parent_nodeid),0,sizeof(int),sizeof(int));

	// Write root's datablocks ~part of metadata~ (one block for now)
	int	move = blocknum*sB.blockSize + sB.filenameSize + sizeof(MDS);
	CALL(lseek(fd,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,data.datablocks,0,sizeof(int),sizeof(int));

	// Write superBlock	
	CALL(lseek(fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
	SAFE_WRITE(fd,&sB,0,sizeof(superBlock),sizeof(superBlock));

	CALL(close(fd),-1,"Error closing file for cfs: ",4,ignore);

	free(data.datablocks);
	return fd;
}

bool cfs_close(int fileDesc) 
{
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