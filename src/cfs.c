/* FILE: cfs.c */

#include "../include/cfs_commands.h"

int main(void) {
	//...
	//scan
	//while command != exit
	//switch
	//run command
	//scan

	char	*ignore = NULL;
	char	*command, line[60], *tmp, *option, *value, *filename;
	int	fileDesc = -1;								//updated by cfs_create
	bool	open_cfs = false;

	do{
		printf("\n\nSelect one of the commands for cfs:\n");
		printf("1. cfs_workwith <FILE>\n"
			"2. cfs_mkdir <DIRECTORIES>\n"
			"3. cfs_touch <OPTIONS> <FILES>\n"
			"4. cfs_pwd\n"
			"5. cfs_cd <PATH>\n"
			"6. cfs_ls <OPTIONS> <FILES>\n"
			"7. cfs_cp <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>\n"
			"8. cfs_cat <SOURCE FILES> -o <OUTPUT FILE>\n"
			"9. cfs_ln <SOURCE FILES> <OUTPUT FILE>\n"
			"10. cfs_mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>\n"
			"11. cfs_rm <OPTIONS> <DESTINATIONS>\n"
			"12. cfs_import <SOURCES> ... <DIRECTORY>\n"
			"13. cfs_export <SOURCES> ... <DIRECTORY>\n"
			"14. cfs_create <OPTIONS> <FILE>\n"
			"15. cfs_exit\n"
			"16. cfs_man <COMMAND>\n\n");

		CALL(fgets(line,sizeof(line),stdin),NULL,NULL,6,ignore);
		tmp = strtok(line,"\n");
		command = strtok(tmp," ");
		if(command == NULL)
			command = tmp;

		if(!strcmp(command,"cfs_workwith"))
		{
			option = strtok(NULL," ");
			if(option == NULL)
				printf("Input error, please give a filename.\n");
			else if(fileDesc == -1)
				printf("%s file does not exist, try command 'cfs_create' first.\n",option);
			else
			{
				cfs_workwith(option);
				open_cfs = true;
				printf("Working with cfs file %s...\n",option);
			}
		}
		else if(!strcmp(command,"cfs_mkdir"))
		{
		}
		else if(!strcmp(command,"cfs_touch"))
		{
			option = strtok(NULL," ");
			if(option == NULL)
				printf("Input error, too few arguments.\n");
			else
			{
				touch_mode	mode;

				while(option != NULL)
				{
					if(!strcmp(option,"-a"))
						mode = ACC;
					else if(!strcmp(option,"-m"))
						mode = MOD;
					else
						mode = CRE;

					option = strtok(NULL," ");
					if(option == NULL)
						printf("Input error, please give a filename.\n");
					else
					{
						bool	touched;

						touched = cfs_touch(fileDesc,option,mode);
						if(touched)
							printf("File %s touched in cfs.\n",option);
					}

					option = strtok(NULL," ");
				}
			}
		}
		else if(!strcmp(command,"cfs_pwd"))
		{
		}
		else if(!strcmp(command,"cfs_cd"))
		{
		}
		else if(!strcmp(command,"cfs_ls"))
		{
		}
		else if(!strcmp(command,"cfs_cp"))
		{
		}
		else if(!strcmp(command,"cfs_cat"))
		{
		}
		else if(!strcmp(command,"cfs_ln"))
		{
		}
		else if(!strcmp(command,"cfs_mv"))
		{
		}
		else if(!strcmp(command,"cfs_rm"))
		{
		}
		else if(!strcmp(command,"cfs_import"))
		{
		}
		else if(!strcmp(command,"cfs_export"))
		{
		}
		else if(!strcmp(command,"cfs_create"))
		{
			int	bSize = -1, filenameSize = -1, maxFSize = -1, maxDirFileNum = -1;
			int	nameSize = FILENAME_SIZE;

			option = strtok(NULL," ");
			if(option == NULL)
				printf("Input error, too few arguments.\n");
			else
			{
				while(option != NULL)
				{
					if(!strcmp(option,"-bs"))
					{
						value = strtok(NULL," ");
						bSize = atoi(value);
					}
					else if(!strcmp(option,"-fns"))
					{
						value = strtok(NULL," ");
						filenameSize = atoi(value);
						nameSize = filenameSize;
					}
					else if(!strcmp(option,"-cfs"))
					{
						value = strtok(NULL," ");
						maxFSize = atoi(value);
					}
					else if(!strcmp(option,"-mdfn"))
					{
						value = strtok(NULL," ");
						maxDirFileNum = atoi(value);
					}
					else if(option != NULL)
					{
						filename = (char*)malloc(nameSize*sizeof(char));
						strcpy(filename,option);
					}
					option = strtok(NULL," ");
				}

				fileDesc = cfs_create(filename,bSize,filenameSize,maxFSize,maxDirFileNum);
				printf("Cfs file %s created.\n",filename);
			}
		}
		else if(!strcmp(command,"cfs_exit"))
		{
			int ignore = 0;
			if(fileDesc != -1 && open_cfs == true)
				CALL(close(fileDesc),-1,"Error closing file for cfs: ",4,ignore);
			open_cfs = false;

			if(fileDesc != -1)
				free(filename);
		}
		else if(!strcmp(command,"cfs_man"))
		{
		}
		else
			printf("Command not found, please try again.\n");
	} while(strcmp(command,"cfs_exit"));

	return 0;
}
