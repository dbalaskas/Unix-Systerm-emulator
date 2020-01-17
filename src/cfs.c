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
	char	*command, line[60], *tmp, *option, *value;
	int	fileDesc = -1;						//updated by cfs_create or cfs_workwith
	bool	open_cfs = false, create_called = false, workwith_called = false;

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
			else
			{
				fileDesc = cfs_workwith(option,create_called);
				open_cfs = true;
				workwith_called = true;
				printf("Working with cfs file %s...\n",option);
			}
		}
		else if(!strcmp(command,"cfs_mkdir"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_touch"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
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

						if(mode != CRE)
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
		}
		else if(!strcmp(command,"cfs_pwd"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_cd"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_ls"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_cp"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_cat"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_ln"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_mv"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_rm"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_import"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_export"))
		{
			if(!workwith_called)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_create"))
		{
			int	bSize = -1, filenameSize = -1, maxFSize = -1, maxDirFileNum = -1;
			int	nameSize = FILENAME_SIZE;
			char	*filename;

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
				create_called = true;
				printf("Cfs file %s created.\n",filename);
				free(filename);
			}
		}
		else if(!strcmp(command,"cfs_exit"))
		{
			int ignore = 0;
			if(fileDesc != -1 && open_cfs == true)
			{
				update_superBlock(fileDesc);
				CALL(close(fileDesc),-1,"Error closing file for cfs: ",4,ignore);
				open_cfs = false;
			}
		}
		else if(!strcmp(command,"cfs_man"))
		{
		}
		else
			printf("Command not found, please try again.\n");
	} while(strcmp(command,"cfs_exit"));

	return 0;
}
