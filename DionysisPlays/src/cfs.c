/* FILE: cfs.c */

#include "../include/cfs_commands.h"

void printCommands();

int main(void) {
	char	*ignore = NULL;
	char	*command, *temp, *option, *value, *rest;
	char	 line[256], name[256];
	int	 fileDesc = -1;						//updated by cfs_create or cfs_workwith
	bool	 open_cfs = false;

	printCommands();
	do{
		printf("\nCFS:");
		fflush( stdout );
		if (open_cfs == true) {
			cfs_pwd();
			fflush( stdout );
		}
		printf("$ ");

		CALL(fgets(line,sizeof(line),stdin),NULL,NULL,6,ignore);
		temp = strtok(line,"\n");
		rest = temp;
		command = strtok_r(temp," \t",&rest);
		if(command == NULL) {
			continue;
		}
		if(!strcmp(command,"cfs_workwith"))
		{
			option = strtok_r(NULL," \t",&rest);
			if(option == NULL)
				printf("Input error, please give a filename.\n");
			else
			{
				if (open_cfs != true) {
					int len = strlen(option);
					char *last_four = &option[len-4];
					if(strcmp(last_four, ".cfs") == 0) {
						fileDesc = cfs_workwith(option);
						if (fileDesc != -1) {
							open_cfs = true;
							printf("Working with cfs file %s...\n",option);
						}
					} else {
						printf("Not a valid cfs file.\n");
					}
				} else {
        				printf("Error: a cfs file is already open. Close it if you want to open other cfs file.\n");
				}
			}
		}
		else if(!strcmp(command,"cfs_mkdir"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok_r(NULL," \t",&rest);
				if(option == NULL)
					printf("Input error, too few arguments.\n");

				int	created_dir;

				while(option != NULL)
				{
					strcpy(name,option);
					created_dir = cfs_mkdir(fileDesc,name);
					if(created_dir != -1)
						printf("Directory %s created.\n",option);

					option = strtok_r(rest," \t",&rest);
				}
			}
		}
		else if(!strcmp(command,"cfs_touch"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok_r(NULL," \t",&rest);
				if(option == NULL)
					printf("Input error, too few arguments.\n");

				int		touched;
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
						option = strtok_r(NULL," \t",&rest);
					if(option == NULL)
						printf("Input error, please give a filename.\n");
					else
					{
						strcpy(name,option);
						touched = cfs_touch(fileDesc,name,mode);
						if(touched != -1)
							printf("File %s touched in cfs.\n",option);
					}

					option = strtok_r(rest," \t",&rest);
				}
			}
		}
		else if(!strcmp(command,"cfs_pwd"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok_r(NULL," \t",&rest);
				if(option != NULL)
					printf("Input error, too many arguments.\n");
				else
					cfs_pwd();
			}
		}
		else if(!strcmp(command,"cfs_cd"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok_r(NULL," \t",&rest);
				if((rest != NULL) && (strcmp(rest,"")))
					printf("Input error, too many arguments.\n");
				else
					cfs_cd(fileDesc, option);
			}
		}
		else if(!strcmp(command,"cfs_ls"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool ls_modes[6];
				string_List *directories = NULL;
				for (int i=0; i<6;i++)
					ls_modes[i]=false;
					
				option = strtok_r(NULL," \t",&rest);

				while(option != NULL)
				{
					if (strcmp(option, "-a") == 0) {
						ls_modes[LS_A] = true;
					} else if (strcmp(option, "-r") == 0) {
						ls_modes[LS_R] = true;
					} else if (strcmp(option, "-l") == 0) {
						ls_modes[LS_L] = true;
					} else if (strcmp(option, "-u") == 0) {
						ls_modes[LS_U] = true;
					} else if (strcmp(option, "-d") == 0) {
						ls_modes[LS_D] = true;
					} else if (strcmp(option, "-h") == 0) {
						ls_modes[LS_H] = true;
					} else {
						add_stringNode(&directories, option);
					}

					option = strtok_r(NULL," \t",&rest);
				}
				if (directories == NULL) {
					cfs_ls(fileDesc, ls_modes, NULL);
				} else {
					while (directories != NULL)
						cfs_ls(fileDesc, ls_modes, pop_last_string(&directories));
				}
			}
		}
		else if(!strcmp(command,"cfs_cp"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_cat"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool		appended;
				char		*option_prev = NULL;
				string_List	*sourceList = NULL;

				option_prev = strtok_r(NULL," \t",&rest);
				option = strtok_r(NULL," \t",&rest);
				// At least to files required
				if(option_prev == NULL || option == NULL)
					printf("Input error, too few arguments.\n");

				while(option_prev != NULL && option != NULL)
				{
					add_stringNode(&sourceList,option_prev);
					option_prev = option;
					option = strtok_r(NULL," \t",&rest);
					if(option == NULL)
					{
						appended = cfs_cat(fileDesc,&sourceList,option_prev);
						if(appended)
							printf("List of sources appended to output file %s.\n",option_prev);
						option_prev = NULL;
					}
				}

				destroy_stringList(sourceList);
			}
		}
		else if(!strcmp(command,"cfs_ln"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_mv"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_rm"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_import"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
			}
		}
		else if(!strcmp(command,"cfs_export"))
		{
			if(open_cfs == false)
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

			option = strtok_r(NULL," \t",&rest);
			if(option == NULL)
				printf("Input error, too few arguments.\n");
			else
			{
				filename = NULL;
				while(option != NULL)
				{
					if(!strcmp(option,"-bs"))
					{
						value = strtok_r(NULL," \t",&rest);
						bSize = atoi(value);
					}
					else if(!strcmp(option,"-fns"))
					{
						value = strtok_r(NULL," \t",&rest);
						filenameSize = atoi(value);
						nameSize = filenameSize;
					}
					else if(!strcmp(option,"-cfs"))
					{
						value = strtok_r(NULL," \t",&rest);
						maxFSize = atoi(value);
					}
					else if(!strcmp(option,"-mdfn"))
					{
						value = strtok_r(NULL," \t",&rest);
						maxDirFileNum = atoi(value);
					}
					else if(option != NULL)
					{
						filename = (char*)malloc(nameSize*sizeof(char));
						strcpy(filename,option);
					}
					option = strtok_r(NULL," \t",&rest);
				}
				if (filename == NULL) {
					printf("Input error, too few arguments.\n");
				} else {
					if (access( filename, F_OK ) != -1){
						printf("cfs file already exists\n");
					} else {
						int len = strlen(filename);
        	                               	char *last_four = &filename[len-4];
	                                       	if(strcmp(last_four, ".cfs") != 0) {
							strncat(filename, ".cfs", 4);
						}
						fileDesc = cfs_create(filename,bSize,filenameSize,maxFSize,maxDirFileNum);
						if(fileDesc != -1)
							printf("Cfs file %s created.\n",filename);
					}
					free(filename);
				}
			}
		}
		else if(!strcmp(command,"cfs_close"))
		{
			if (open_cfs == true) {
				cfs_close(fileDesc);
				open_cfs = false;
				printf("cfs file is closed.\n");
			}
			else{
				printf("There is no cfs file opened.\n");
			}
		}
		else if(!strcmp(command,"cfs_man"))
		{
		}
		else if(!strcmp(command,"help"))
		{
			printCommands();
		}
		else if(!strcmp(command,"cfs_exit"))
		{
			if(open_cfs == true)
				cfs_close(fileDesc);
			printf("End of Program.\n");
			return 0;
		}
		else
			printf("Command not found, please try again.\n");
	} while(1);
}

void printCommands(){
	printf( " 1. cfs_workwith <FILE>\n"
		" 2. cfs_mkdir <DIRECTORIES>\n"
		" 3. cfs_touch <OPTIONS> <FILES>\n"
		" 4. cfs_pwd\n"
		" 5. cfs_cd <PATH>\n"
		" 6. cfs_ls <OPTIONS> <FILES>\n"
		" 7. cfs_cp <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>\n"
		" 8. cfs_cat <SOURCE FILES> -o <OUTPUT FILE>\n"
		" 9. cfs_ln <SOURCE FILES> <OUTPUT FILE>\n"
		"10. cfs_mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>\n"
		"11. cfs_rm <OPTIONS> <DESTINATIONS>\n"
		"12. cfs_import <SOURCES> ... <DIRECTORY>\n"
		"13. cfs_export <SOURCES> ... <DIRECTORY>\n"
		"14. cfs_create <OPTIONS> <FILE>\n"
		"15. cfs_man <COMMAND>\n"
		"16. cfs_close\n"
		"17. help\n"
		"18. cfs_exit\n");
}
