/* FILE: cfs.c */

#include "../include/cfs_commands.h"

void printCommands();

int main(void) {
	int 	 i;
	char	*ignore = NULL;
	char	*command, *tmp, *option, *value;
	char	 line[256], name[FILENAME_SIZE];
	int		 fileDesc = -1;						//updated by cfs_create or cfs_workwith
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
		tmp = strtok(line,"\n");
		command = strtok(tmp," \t");
		if(command == NULL) {
			continue;
		}
		if(!strcmp(command,"cfs_workwith"))
		{
			option = strtok(NULL," \t");
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
			}
		}
		else if(!strcmp(command,"cfs_touch"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				option = strtok(NULL," \t");
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
							option = strtok(NULL," \t");
						if(option == NULL)
							printf("Input error, please give a filename.\n");
						else
						{
							bool	touched;

							strcpy(name,option);
							touched = cfs_touch(fileDesc,name,mode);
							if(touched)
								printf("File %s touched in cfs.\n",option);
						}

				 		if(option != NULL)
							option = strtok(NULL," \t");
//						if(option != NULL)
//							printf("%s\n",option);
					}
				}
			}
		}
		else if(!strcmp(command,"cfs_pwd"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				cfs_pwd();
			}
		}
		else if(!strcmp(command,"cfs_cd"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				char *path = strtok(NULL," \t");
				cfs_cd(fileDesc, path);
			}
		}
		else if(!strcmp(command,"cfs_ls"))
		{
			if(open_cfs == false)
				printf("Cfs closed, try cfs_workwith first.\n");
			else
			{
				char path[256];
				bool ls_modes[6];
				for (int i=0; i<6;i++)
					ls_modes[i]=false;
				
				option = strtok(NULL," \t");
				if (option == NULL) {
					cfs_ls(fileDesc, ls_modes, NULL);
				}

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
					// } else {
					// 	// i=0;
					// 	// while (ls_modes[i] == false && i<6) {
					// 	// 	i++;
					// 	// }

					// 	// if (option == NULL && i < 6) {
					// 	// 	printf("Input error, please give a filename.\n");
					// 	// } else {
					// 	// 	strcpy(path,option);
					// 	// 	cfs_ls(fileDesc, ls_modes, path);
					// 	// }

					// 	strcpy(path,option);
					// 	cfs_ls(fileDesc, ls_modes, path);

					// 	for (int i=0; i<6;i++)
					// 		ls_modes[i]=false;
					// }
				}
				i=0;
				while (ls_modes[i] == false && i<6) {
					i++;
				}
				if (i < 6) {
					cfs_ls(fileDesc, ls_modes, NULL);
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

			option = strtok(NULL," \t");
			if(option == NULL)
				printf("Input error, too few arguments.\n");
			else
			{
				filename = NULL;
				while(option != NULL)
				{
					if(!strcmp(option,"-bs"))
					{
						value = strtok(NULL," \t");
						bSize = atoi(value);
					}
					else if(!strcmp(option,"-fns"))
					{
						value = strtok(NULL," \t");
						filenameSize = atoi(value);
						nameSize = filenameSize;
					}
					else if(!strcmp(option,"-cfs"))
					{
						value = strtok(NULL," \t");
						maxFSize = atoi(value);
					}
					else if(!strcmp(option,"-mdfn"))
					{
						value = strtok(NULL," \t");
						maxDirFileNum = atoi(value);
					}
					else if(option != NULL)
					{
						filename = (char*)malloc(nameSize*sizeof(char));
						strcpy(filename,option);
					}
					option = strtok(NULL," \t");
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
