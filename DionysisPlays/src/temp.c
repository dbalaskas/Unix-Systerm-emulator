bool cfs_import(int fileDesc,string_List *sourceList,char *destPath)
{
	int		start;
	int		source_nodeid, dest_nodeid, dest_entities, parent_nodeid;
	char		*parent_name, *new_path, *initial;
	MDS		*source_mds, *dest_mds;
	Datastream	data;

	// Find the nodeid of the first entity in 'destPath'
	start = getPathStartId(destPath);
	// Get the nodeid of the entity the destination path leads to
	dest_nodeid = traverse_cfs(fileDesc,destPath,start);
	// If destination path was invalid
	if(dest_nodeid == -1)
		return false;
	// Get destination directory's metadata
	dest_mds = (MDS*) (inodeTable + dest_nodeid*inodeSize + sizeof(bool) + sB.filenameSize);
	if(dest_mds->type != Directory)
	{
		printf("Input error, destination path %s is not a directory.\n",inodeTable+dest_nodeid*inodeSize+sizeof(bool));
		return false;
	}
	dest_entities = getDirEntitiesNum(fileDesc,destPath);
//	parent_nodeid = get_parent(fileDesc,destPath,NULL);
//	parent_name = inodeTable + parent_nodeid*inodeSize + sizeof(bool);

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

	int		source_end, current;
	int		dataCounter, move;
	char		dirBuffer[sB.filenameSize+sizeof(int)];
	char		fileBuffer[sB.blockSize];
	struct stat	type;
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
			strcat(new_path,destPath);
			strcat(new_path,split);
			free(initial);

			// Try to get source's type
			if(stat(source_name,&type) == -1)
			{
				perror("Error with stat in source entity: ");
				free(source_name);
				return false;
			}
			// If it is a directory
			if(S_ISDIR(type.st_mode))
//			if((source_fd = open(source_name, O_DIRECTORY | O_RDONLY)) != -1)
			{
				bool		readnext = false;
				DIR		*dir;
				struct dirent	*content;

				if((dir = opendir(source_name)) == NULL)
				{
					perror("Error opening source directory: ");
					free(source_name);
					return false;
				}

//				// Get directory's size
//				CALL(lseek(source_fd,0,SEEK_END),-1,"Error moving ptr in cfs file: ",5,source_end);
//				// Go back to the beginning and read directory's contents
//				CALL(lseek(source_fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
//				SAFE_READ(source_fd,dirBuffer,0,sizeof(char),sB.filenameSize+sizeof(int));

				// Create a new directory in cfs (under the appropriate parent directory)
				source_nodeid = cfs_mkdir(fileDesc,new_path);
				if(source_nodeid == -1)
				{
					free(source_name);
					return false;
				}
				source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool));
				data.datablocks = (int*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sizeof(MDS));

				content = readdir(dir);
				memcpy(dirBuffer,content->d_name,strlen(content->d_name)+1);
				memcpy(dirBuffer+sB.filenameSize,&source_nodeid,sizeof(int));

				// Fill directory's data
				for(int j=0; j<(sB.maxFileDatablockNum); j++)
				{
					// If current datablock is not empty
					if(data.datablocks[j] != -1)
					{
						move = data.datablocks[j]*sB.blockSize;
						CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(fd,&dataCounter,0,sizeof(int),sizeof(int));
						// If current datablock is not full
						if(dataCounter < sB.maxEntitiesPerBlock)
						{
							// Write source directory's contents
							move = dataCounter*(sB.filenameSize + sizeof(int));
							CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
							SAFE_WRITE(fileDesc,dirBuffer,0,sizeof(char),sB.filenameSize+sizeof(int));

							// Increase counter in datablock
							dataCounter++;
							move = data.datablocks[j]*sB.blockSize;
							CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
							SAFE_WRITE(fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
							// We can now read source directory's next contents
							readnext = true;
						}
					}
					else
					{
						data.datablocks[j] = getEmptyBlock();
						// Directory's datablocks are increased by one
						source_mds->datablocksCounter++;
						move = data.datablocks[j]*sB.blockSize;
						CALL(lseek(fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						dataCounter = 1;
						// Set counter in datablock
						SAFE_WRITE(fileDesc,&dataCounter,0,sizeof(int),sizeof(int));
						// Write source directory's contents
						SAFE_WRITE(fileDesc,dirBuffer,0,sizeof(char),sB.filenameSize+sizeof(int));
						// We can now read source directory's next contents
						readnext = true;
					}

					if(readnext)
					{
						// Get current location in source directory
						CALL(lseek(source_fd,0,SEEK_CUR),-1,"Error moving ptr in cfs file: ",5,current);
						// If there are still info to read
						if((current + sB.filenameSize+sizeof(int)) < source_end)
							SAFE_READ(source_fd,dirBuffer,0,sizeof(char),sB.filenameSize+sizeof(int));
						// Reached end of directory
						else
							break;
					}
				}
			}
			// If it is a file
			else if(S_ISREG(type.st_mode))
//			else if((source_fd = open(source_name, 0_RDONLY)) != -1)
			{
				int	size_to_read = sB.blockSize;

				// Get file's size
				CALL(lseek(source_fd,0,SEEK_END),-1,"Error moving ptr in cfs file: ",5,source_end);
				// Go back to the beginning and read file's contents
				CALL(lseek(source_fd,0,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
				SAFE_READ(source_fd,fileBuffer,0,sizeof(char),size_to_read);
				// Create a new file in cfs (under the appropriate parent directory)
				source_nodeid = cfs_touch(fileDesc,new_path,CRE);
				if(source_nodeid == -1)
				{
					free(source_name);
					return false;
				}
				source_mds = (MDS*) (inodeTable + source_nodeid*inodeSize + sizeof(bool));
				data.datablocks = (int*) (inodeTable + source_nodeid*inodeSize + sizeof(bool) + sizeof(MDS));
				// Fill file's data
				for(int j=0; j<(sB.maxFileDatablockNum); j++)
				{
					// If current datablock is empty
					if(data.datablocks[j] == -1)
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
							SAFE_READ(source_fd,dirBuffer,0,sizeof(char),size_to_read);
						}
						// Reached end of directory
						else
							break;
					}
				}
			}
//			else	// source_fd == -1
//			{
//				perror("Error opening source entity: ");
//				free(source_name);
//				return false;
//			}
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
}

