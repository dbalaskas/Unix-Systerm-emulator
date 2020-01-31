/*int getPathStartId(cfs_info *info, char* path)
{
	int start;
	char initial_path[strlen(path)+1];
	if(path[0] == '/')
	{
		// Traversing cfs will start from the root (nodeid = 0)
		start = 0;
	}
	else
	{
		// Get first entity in path
		strcpy(initial_path,path);
//		char *start_dir = strtok(initial_path,"/");
		// If path starts from current directory's parent
//		if(strcmp(start_dir,"..") == 0)
//		{
//			MDS *current_mds = (MDS *) (info->inodeTable + (info->cfs_current_nodeid)*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
//			current_mds->access_time = time(NULL);
//			start = current_mds->parent_nodeid;
//		}
//		else	//if(!strcmp(split,".") || strcmp(split,"/"))
//		{
			// If path starts from current directory
			start = (info->cfs_current_nodeid);
//		}
	}

	return start;
}

int traverse_cfs(cfs_info *info,char *filename)
{
	int		start, curr_nodeid;
	int		offset, i, move;
	int		blocknum, datablocks_checked, dataCounter, j, ignore = 0;
	char		*split;
	char		path_name[(info->sB).filenameSize];
	char		*curr_name = (char*)malloc((info->sB).filenameSize*sizeof(char));
	MDS		*metadata, *parent_mds;
	Datastream	data;

	// 'path_name' will be used by strtok, so that 'filename' remains unchanged
	strcpy(path_name,filename);
	// If path starts from the root
	if(path_name[0] == '/')
		start = (info->sB).root;
	// In any other case, traverse should start from current nodeid
	else
		start = cfs_current_nodeid;

	// Get first entity in path
	split = strtok(path_name,"/");

	// Go to the element of the inodeTable with nodeid equal to 'start' (path's first entity)
	offset = start*(info->inodeSize) + sizeof(bool);

	// As far as there is another entity in path
	while(split != NULL)
	{
		// If current entity is "."
		if(!strcmp(split,"."))
			split = info->inodeTable + (info->cfs_current_nodeid)*(info->inodeSize) + sizeof(bool);
		// If current entity is ".."
		else if(!strcmp(split,".."))
		{
			metadata = (MDS*) (info->inodeTable + (info->cfs_current_nodeid)*(info->inodeSize) + sizeof(bool) + (info->sB).filenameSize);
			metadata->access_time = time(NULL);
			split = info->inodeTable + (metadata->parent_nodeid)*(info->inodeSize) + sizeof(bool);
		}
		// Go to  entity's metadata and data
		offset += (info->sB).filenameSize;
		metadata = (MDS*) (info->inodeTable + offset);
		metadata->access_time = time(NULL);
		offset += sizeof(MDS);
		data.datablocks = (int*) (info->inodeTable + offset);
		// If it is a directory
		if(metadata->type == Directory)
		{
			// Number of datablocks we have searched
			datablocks_checked = 0;
			// For directory's each data block
			for(i=0; i<(info->sB).maxFileDatablockNum; i++)
			{
				// If it has contents
				if(data.datablocks[i] != -1)
				{
					// Check directory's contents
					blocknum = data.datablocks[i];
					CALL(lseek(info->fileDesc,blocknum*(info->sB).blockSize,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
					SAFE_READ(info->fileDesc,&dataCounter,0,sizeof(int),sizeof(int));

					// For every pair of (filename+nodeid) in datablock
					for(j=0; j<dataCounter; j++)
					{
						move = blocknum*(info->sB).blockSize + sizeof(int) + j*((info->sB).filenameSize+sizeof(int));
						CALL(lseek(info->fileDesc,move,SEEK_SET),-1,"Error moving ptr in cfs file: ",5,ignore);
						SAFE_READ(info->fileDesc,curr_name,0,sizeof(char),(info->sB).filenameSize);
						SAFE_READ(info->fileDesc,&curr_nodeid,0,sizeof(int),sizeof(int));
						if(!strcmp(curr_name,".") || !strcmp(curr_name,".."))
							strcpy(curr_name, info->inodeTable + curr_nodeid*(info->inodeSize) + sizeof(bool));

						// If current directory contains the entity we are looking for
						if(!strcmp(split,curr_name))
						{
							// Keep entity's place in inodeTable
							start = curr_nodeid;
							offset = start*(info->inodeSize) + sizeof(bool);
							break;
						}
					}
					// If entity was found
					if(j < dataCounter)
						break;
					// Go to the next datablock
					datablocks_checked++;
				}
				// If there are no more datablocks with contents
				if(datablocks_checked == metadata->datablocksCounter)
					break;
			}
			// If entity wasn't found
			if(i == (info->sB).maxFileDatablockNum || datablocks_checked == metadata->datablocksCounter)
			{
				free(curr_name);
				return -1;
			}
		}
		// If current entity is a file
		else if(metadata->type == File)
		{
			split = strtok(NULL,"/");
			// But there is another entity in path
			if(split != NULL)
			{
				free(curr_name);
				return -1;
			}
			// Else, it is the file we were looking for ('start' has already been appropriately updated)
			break;
		}

		// Get next entity's name in path
		split = strtok(NULL,"/");
	}

	free(curr_name);
	return start;
}
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cleanSlashes(char **path)
{
	char	*name = (char*)malloc(1);
	int	i, size = 1;

	for(i=0; i<(strlen(*path)); i++)
	{
		if((*path)[i] == '/' && (*path)[i+1] == '/')
			continue;
		else
		{
			name = realloc(name,++size);
			name[size-2] = (*path)[i];
		}
	}
	name[i] = '\0';
	strcpy(*path,name);
	free(name);
}

int main()
{
	char *path = (char*)malloc(100);
	strcpy(path,"//");
	cleanSlashes(&path);
	printf("%s\n",path);
	free(path);
	return 0;
}
