/* FILE: cfs.c */

#include "../include/cfs_commands.h"

void printCommands();

int main(void) {
	char		*ignore = NULL;
	char		*command, *temp, *option, *value, *rest;
	char	 	line[256], name[256];
	int	 	fileDesc = -1;						//updated by cfs_create or cfs_workwith
	bool	 	open_cfs = false;
	cfs_info	cfsInfo;

	// Initialize cfsInfo
	cfsInfo.fileName = NULL;
	cfsInfo.inodeTable = NULL;
	cfsInfo.holes = NULL;

	printf("\n");
	printCommands();
	do{
		printf("\033[3;32mCFS:");
		fflush( stdout );
		if (open_cfs == true) {
			printf("\033[6;31m");
			cfs_pwd();
			fflush( stdout );
		}
		printf("\033[6;31m$ ");
		printf("\033[0m");

		CALL(fgets(line,sizeof(line),stdin),NULL,NULL,6,ignore);
		temp = strtok(line,"\n");
		rest = temp;
		if(rest == NULL)
			continue;
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
						strcpy(cfsInfo.fileName,option);
						fileDesc = cfs_workwith(&cfsInfo);
						if (cfsInfo.fileDesc != -1) {
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
					created_dir = cfs_mkdir(&cfsInfo,name);
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
						touched = cfs_touch(&cfsInfo,name,mode);
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
					cfs_cd(&cfsInfo, option);
			}
		}
		else if(!strcmp(command,"cfs_ls"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool ls_modes[ls_mode_Num];
				char *dirname;
				string_List *directories = NULL;
				for (int i=0; i<ls_mode_Num;i++)
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
					cfs_ls(&cfsInfo, ls_modes, NULL);
				} else {
					while (directories != NULL) {
						if((dirname = pop_last_string(&directories)) != NULL) {
							cfs_ls(&cfsInfo, ls_modes, dirname);
							free(dirname);
						}
					}
				}
				printf("\n");
			}
		}
		else if(!strcmp(command,"cfs_cp"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool cp_modes[cp_mode_Num];
				string_List *sources = NULL;
				char * destination;
				for (int i=0; i<cp_mode_Num;i++)
					cp_modes[i]=false;
					
				option = strtok_r(NULL," \t",&rest);

				while(option != NULL)
				{
					if (strcmp(option, "-r") == 0) {
						cp_modes[CP_R] = true;
					} else if (strcmp(option, "-i") == 0) {
						cp_modes[CP_I] = true;
					} else if (strcmp(option, "-R") == 0) {
						cp_modes[CP_RR] = true;
					} else {
						add_stringNode(&sources, option);
					}

					option = strtok_r(NULL," \t",&rest);
				}
				if (sources == NULL) {
					printf("cp: missing file operand\n");
				} else {
					destination = pop_string(&sources);
					if (sources == NULL) 
						printf("cp: missing destination file operand after '%s'\n", destination);
					else
						cfs_cp(&cfsInfo, cp_modes, sources, destination);
				}
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
				// At least two files required
				if(option_prev == NULL || option == NULL)
					printf("Input error, too few arguments.\n");

				while(option_prev != NULL && option != NULL)
				{
					add_stringNode(&sourceList,option_prev);
					option_prev = option;
					option = strtok_r(NULL," \t",&rest);
					if(option == NULL)
					{
						appended = cfs_cat(&cfsInfo,sourceList,option_prev);
						if(appended)
						{
							printf("List of sources appended to output file %s.\n",option_prev);
							sourceList = NULL;
						}
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
				if ((option = strtok_r(NULL," \t",&rest)) == NULL) {
					printf("ln: missing file operand\n");
				} else if ((value = strtok_r(NULL," \t",&rest)) == NULL) {
					printf("ln: missing destination file operand after '%s'\n", option);					
				} else {
					cfs_ln(&cfsInfo, option, value);
				}
			}
		}
		else if(!strcmp(command,"cfs_mv"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool mv_modes[mv_mode_Num];
				string_List *sources = NULL;
				char * destination;
				for (int i=0; i<mv_mode_Num;i++)
					mv_modes[i]=false;
					
				option = strtok_r(NULL," \t",&rest);

				while(option != NULL)
				{
					if (strcmp(option, "-i") == 0) {
						mv_modes[CP_I] = true;
					} else {
						add_stringNode(&sources, option);
					}

					option = strtok_r(NULL," \t",&rest);
				}
				if (sources == NULL) {
					printf("mv: missing file operand\n");
				} else {
					destination = pop_string(&sources);
					if (sources == NULL) 
						printf("mv: missing destination file operand after '%s'\n", destination);
					else
						cfs_mv(&cfsInfo, mv_modes, sources, destination);
				}
			}
		}
		else if(!strcmp(command,"cfs_rm"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool		rm_modes[2];
				bool		removed;
				char		*dirname;
				string_List	*directories = NULL;
				for (int i=0; i<2;i++)
					rm_modes[i] = false;

				option = strtok_r(NULL," \t",&rest);

				while(option != NULL)
				{
					if(!strcmp(option,"-i"))
					{
						rm_modes[RM_I] = true;
					}
					else if(!strcmp(option,"-r"))
					{
						rm_modes[RM_R] = true;
					}
					else
						add_stringNode(&directories, option);

					option = strtok_r(NULL," \t",&rest);
				}
				if(directories == NULL)
					printf("Input error, please give a directory name.\n");
				else
				{
					while (directories != NULL)
					{
						if((dirname = pop_last_string(&directories)) != NULL)
						{
							removed = cfs_rm(&cfsInfo,rm_modes,dirname);
							if(removed)
								printf("Removed %s's contents from cfs.\n",dirname);

							free(dirname);
						}
					}
				}
			}
		}
		else if(!strcmp(command,"cfs_import"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool		imported;
				char		*option_prev = NULL;
				string_List	*sourceList = NULL;

				option_prev = strtok_r(NULL," \t",&rest);
				option = strtok_r(NULL," \t",&rest);
				// At least two files required
				if(option_prev == NULL || option == NULL)
					printf("Input error, too few arguments.\n");

				while(option_prev != NULL && option != NULL)
				{
					add_stringNode(&sourceList,option_prev);
					option_prev = option;
					option = strtok_r(NULL," \t",&rest);
					if(option == NULL)
					{
						imported = cfs_import(&cfsInfo,sourceList,option_prev);
						if(imported)
						{
							printf("List of sources imported to output directory %s.\n",option_prev);
							sourceList = NULL;
						}
						option_prev = NULL;
					}
				}
				destroy_stringList(sourceList);
			}
		}
		else if(!strcmp(command,"cfs_export"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				bool		exported;
				char		*option_prev = NULL;
				string_List	*sourceList = NULL;

				option_prev = strtok_r(NULL," \t",&rest);
				option = strtok_r(NULL," \t",&rest);
				// At least two files required
				if(option_prev == NULL || option == NULL)
					printf("Input error, too few arguments.\n");

				while(option_prev != NULL && option != NULL)
				{
					add_stringNode(&sourceList,option_prev);
					option_prev = option;
					option = strtok_r(NULL," \t",&rest);
					if(option == NULL)
					{
						exported = cfs_export(&cfsInfo,sourceList,option_prev);
						if(exported)
						{
							printf("List of sources exported to output directory %s.\n",option_prev);
							sourceList = NULL;
						}
						option_prev = NULL;
					}
				}
				destroy_stringList(sourceList);
			}
		}
		else if(!strcmp(command,"cfs_create"))
		{
			int	bSize = -1, filenameSize = -1, maxFSize = -1, maxDirFileNum = -1;
			int	nameSize = FILENAME_SIZE;

			option = strtok_r(NULL," \t",&rest);
			if(option == NULL)
				printf("Input error, too few arguments.\n");
			else
			{
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
						cfsInfo.fileName = (char*)malloc(nameSize*sizeof(char));
						strcpy(cfsInfo.fileName,option);
					}
					option = strtok_r(NULL," \t",&rest);
				}
				if (cfsInfo.fileName == NULL) {
					printf("Input error, too few arguments.\n");
				} else {
					if (access( cfsInfo.fileName, F_OK ) != -1){
						printf("cfs file already exists\n");
					} else {
						int len = strlen(cfsInfo.fileName);
        	                               	char *last_four = &(cfsInfo.fileName)[len-4];
	                                       	if(strcmp(last_four, ".cfs") != 0) {
							strncat(cfsInfo.fileName, ".cfs", 4);
						}
						cfsInfo.fileDesc = cfs_create(&cfsInfo,bSize,filenameSize,maxFSize,maxDirFileNum);
						if(cfsInfo.fileDesc != -1)
							printf("Cfs file %s created.\n",cfsInfo.fileName);
					}
				}
			}
		}
		else if(!strcmp(command,"cfs_close"))
		{
			if (open_cfs == true) {
				cfs_close(&cfsInfo);
				open_cfs = false;
				printf("cfs file is closed.\n");
			}
			else{
				printf("There is no cfs file opened.\n");
			}
		}
		else if(!strcmp(command,"cfs_print"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok_r(NULL," \t",&rest);
				if((rest != NULL) && (strcmp(rest,"")))
					printf("Input error, too many arguments.\n");
				else if(option == NULL)
					printf("Input error, please give a path.\n");
				else
				{	// Save possible changes in cfs file and re-open it
					cfs_close(&cfsInfo);
					cfs_workwith(&cfsInfo);
					print_data(&cfsInfo,option);
				}
			}
		}
		else if(!strcmp(command,"help"))
		{
			printCommands();
		}
		else if(!strcmp(command,"cfs_exit"))
		{
			if(open_cfs == true)
				cfs_close(&cfsInfo);
			if(cfsInfo.fileName != NULL)
				free(cfsInfo.fileName);
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
		"15. cfs_print <PATH>\n"
		"16. cfs_close\n"
		"17. help\n"
		"18. cfs_exit\n\n");
}
