/* FILE: cfs_commands.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				//creat, open
#include "../include/cfs_commands.h"

int cfs_workwith(char *filename)
{
	int	fd = -1, ignore = 0;
	int	offset = 0, listOffset;
	int	overflow_block, read_iblocks, metadata_blocknum;
	int	current_iblock[sB.iTableBlocksNum];					//TO BE DISCUSSED!!
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
	// Check if directory exists
	if(traverse_cfs(fd,new_name,parent_nodeid) != -1)
	{
		printf("Input error, directory '%s' already exists.\n",dirname);
		return -1;
	}
	// Go to parent's metadata
	parent_mds = (MDS*) (inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	// Go to parent's data
	parent_data.datablocks = (int*)(inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

	int	dataCounter, move, numofEntities = 0;;

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
			numofEntities += dataCounter;
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
		// Check if file exists
		if(traverse_cfs(fd,new_name,parent_nodeid) != -1)
		{
			printf("Input error, file '%s' already exists.\n",filename);
			return -1;
		}
		// Go to parent's metadata
		parent_mds = (MDS*) (inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		// Go to parent's data
		parent_data.datablocks = (int*)(inodeTable + parent_nodeid*inodeSize + sizeof(bool) + sB.filenameSize + sizeof(MDS));

		int	dataCounter, move, numofEntities = 0;

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
				numofEntities += dataCounter;
			}

			// If parent directory has no datablocks available
			if(parent_mds->datablocksCounter == sB.maxDirDatablockNum && numofEntities == sB.maxEntitiesPerBlock*sB.maxDirDatablockNum)
			{
				printf("Not enough space in parent directory to create file '%s'.\n",filename);
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
			return -1;
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

bool cfs_pwd() {
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

bool cfs_cd(int fileDesc, char *path) {
	int 	 path_nodeId = -1;
	MDS 	*path_MDS = NULL;
	int		 start;

	// Check if the path has value in it.
	if (path != NULL) {
		char	*initial_path = (char*)malloc(strlen(path)+1);

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

		free(initial_path);
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

bool cfs_ls(int fileDesc, bool *ls_modes, char *path) {
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
		char	*initial_path = (char*)malloc(strlen(path)+1);

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

		free(initial_path);

		// Check if the path has valid value.
		if(path_nodeId != -1) {
			path_MDS = (MDS *) (inodeTable + path_nodeId*inodeSize + sizeof(bool) + sB.filenameSize);
		} else {
			printf("Input error, '%s' is not a valid path.\n",path);
			return false;
		}
		// Check if the returned id is a directory. If it is not print error.
		if (path_MDS->type != Directory) {
			printf("Input error, '%s' is not a valid path.\n",path);
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
			if(strcmp(current_filename,".") && strcmp(current_filename,".."))
				addNode(&path_content, current_nodeid);
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
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
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if (current_MDS->type == Directory) {

			}
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
	if (ls_modes[1] == true) {
		char *temp = (char*) malloc(strlen(dirName)+1);
		while (path_content != NULL) {
			strcpy(temp, dirName);
			current_nodeid = pop(&path_content);
			current_filename = inodeTable + current_nodeid*inodeSize + sizeof(bool);
			current_MDS = (MDS *) (inodeTable + current_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if (current_MDS->type == Directory && current_MDS->nodeid != path_nodeId && current_MDS->nodeid != path_MDS->parent_nodeid) {
				printf("\n");
				int dirPathSize = strlen(temp) + strlen(current_filename) + 2;
				temp = realloc(temp, dirPathSize);
				strcat(temp, "/");
				strcat(temp, current_filename);
				
				cfs_ls(fileDesc, ls_modes, temp);
			}
		}
		free(temp);
	}
	free(dirName);
	return true;
}

bool cfs_cp(int fileDesc, cp_mode *cp_modes, string_List *sourceList, char *destination)
{
	int 		 sourcesCount = getLength(sourceList);
	char		 answer;

	char 		*source_path;
	char 		*source_name;
	int 		 source_nodeid;
	MDS 		*source_mds;

	int 		 directory_nodeid;
	// MDS 		*directory_mds;

	char		*destination_path;
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
	// directory_mds 	 = (MDS*) (inodeTable + directory_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	//------------------------------------------------------------------------------------
	// Copy each source to destination.
	while (sourceList != NULL) {
		source_path = pop_last_string(&sourceList);
		// Get source inode.
		source_nodeid = traverse_cfs(fileDesc, source_path, getPathStartId(source_path));
		// Check if source_path has valid value.
		if(source_nodeid == -1) {
			printf("cp: cannot stat '%s': No such file or directory\n",source_path);
			continue;
		}
		source_name = inodeTable + source_nodeid*inodeSize + sizeof(bool);
		source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		//--------------------------------------------------------------------------------
		// Get destination inode.
		// Get id of the destination.
		destination_nodeid = traverse_cfs(fileDesc,destination_name,directory_nodeid);
		if (destination_nodeid == -1) {
			// If the destination does not exists, it has to be imported only one source.
			// Create a new inode at the destination and copy the source in it.
			if (sourcesCount == 1) {
				destination_path = destination;
			} else {
				printf("cp: target '%s' is not a directory\n", destination);
				continue;
			} 
		} else {	// destination_id != -1
			// If the file exists check the its type.
			destination_mds = (MDS*) (inodeTable + destination_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if (destination_mds->type == Directory) {
				// If destination is directory, check if exists an inode in it with the name of the source.
				// If it does, Copy the source to the inode of the destination, Overide it and change it's metadata.
				// Else, Create a new inode at the destination and copy source in it.
				destination_path = (char*) malloc(strlen(destination)+strlen(source_name)+2);
				strcpy(destination_path, destination);
				strcat(destination_path, "/");
				strcat(destination_path, source_name);


				// replace_nodeid = traverse_cfs(fileDesc,source_name,destination_nodeid);
				// if (replace_nodeid == -1) {
				// } else {
				// 	destination_mds = (MDS*) (inodeTable + replace_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
				// 	if (source_mds->type != Directory) {

				// 	} else {

				// 	}
				// 	if (source_mds->type == Directory && destination_mds->type == Directory) {
				// 		destination_path = (char*) malloc(strlen(destination)+strlen(source_name)+2);
				// 		string_List *directory_inodeList;
				// 		//directory_inodeList = ;
				//		cfs_cp(fileDesc, cp_modes, directory_inodeList, destination_path);
				// 		continue;
				// 																		// For same names and different types!!!
				// 																		// } else if (source_mds->type != Directory && destination_mds->type == Directory) {
				// 																		// 	printf("cp: cannot overwrite directory '%s/%s' with non-directory\n", destination, source_name);
				// 																		// 	continue;
				// 																		// } else {
				// 																		// 	printf("cp: cannot overwrite non-directory '%s/%s' with directory '%s'\n", destination, source_name, source_name);
				// 																		// 	continue;
				// 																		// }
				// }
			} else {	// destination_mds->type != Directory
				// If destination is not directory, it has to be imported only one source.
				// Copy the source to the inode of the destination.
				// Overides it and change it's metadata.
				if (source_mds->type != Directory && sourcesCount == 1) {
					destination_path = destination;
				} else {
					printf("cp: target '%s' is not a directory\n", destination);
					continue;
				}
			}
		}
		//--------------------------------------------------------------------------------
		// If mode -i is enabled ask if users want to overwrite th destination.
		if (cp_modes[CP_I]) {
			printf("cp: overwrite '%s/%s'? (y|n)", destination, source_name);
			//...
			// If he doesn't accept, do not copy.
			if (answer == 'n' || answer == 'N')
				continue;
		}
		//--------------------------------------------------------------------------------
		// replace source (source_nodeid) to destination (destination_nodeid).
					destination_mds->type = source_mds->type;
		// Updadte (or create) metadata.
		if (source_mds->type == File) {
			destination_nodeid = cfs_touch(fileDesc, destination_path, CRE);
		} else if (source_mds->type == Link) {
			//destination_nodeid = ln(fileDesc, destination_path);
		} else if (source_mds->type == Directory && cp_modes[CP_R]) {
			destination_nodeid = cfs_mkdir(fileDesc, destination_path);
		} else {
			printf("cp: -r not specified; omitting directory '%s'\n", source_path);
			continue;
		}
		// Write data.
		replaceEntity(fileDesc, source_nodeid, destination_nodeid);
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
		return false;
	// Get output file's size
	output_mds = (MDS*) (inodeTable + output_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	if(output_mds->type != File)
	{
		printf("Input error, wrong output file type.\n");
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
			source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
			if(source_mds->type != File)
			{
				printf("Input error, wrong source file type.\n");
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

	for(int i=1; i<=listSize; i++)
	{
		source_name = pop_last_string(&sourceList,i);
		start = getPathStartId(source_name);
		source_nodeid = traverse_cfs(fileDesc,source_name,start);
		source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
		append_file(fileDesc,source_nodeid,output_nodeid);
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
//find if file exists, if so: syscall remove

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
	if((sB.blockSize - sizeof(int)) % (sB.filenameSize + sizeof(int)))
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