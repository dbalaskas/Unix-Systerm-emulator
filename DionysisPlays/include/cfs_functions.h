/* FILE: cfs_functions.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cfs_file.h"

typedef enum {false, true} 							bool;

typedef enum {CRE, ACC, MOD} 						touch_mode;
#define touch_mode_Num 3
typedef enum {LS_A, LS_R, LS_L, LS_U, LS_D, LS_H} 	ls_mode;
#define ls_mode_Num 6
typedef enum {CP_I, CP_R, CP_RR}			 		cp_mode;
#define cp_mode_Num 3
typedef enum {MV_I}			 						mv_mode;
#define mv_mode_Num 1
typedef enum {RM_I, RM_R}		 					rm_mode;
#define rm_mode_Num 2	

#define CALL(call,error,errString,errNum,returnVal)								\
returnVal = call;																\
if(returnVal == error)															\
{																				\
	if(errString != NULL)														\
		perror(errString);														\
	exit(errNum);																\
}

#define SAFE_READ(fd,ptr,ptr_values,ptr_size,size)								\
{																				\
	int n, offset;																\
	int sum = 0;																\
	while(sum < size)															\
	{																			\
		offset = (ptr_values + sum) / ptr_size;									\
		CALL(read(fd,ptr+offset,size),-1,"Error reading from cfs file: ",2,n);	\
		sum += n;																\
	}																			\
}

#define SAFE_WRITE(fd,ptr,ptr_values,ptr_size,size)								\
{																				\
	int n, offset;																\
	int sum = 0;																\
	while(sum < size)															\
	{																			\
		offset = (ptr_values + sum) / ptr_size;									\
		CALL(write(fd,ptr+offset,size),-1,"Error writing in cfs file: ",3,n);	\
		sum += n;																\
	}																			\
}

void update_superBlock(cfs_info*);
int traverse_cfs(cfs_info*, char*,int);

int getTableSpace(cfs_info*);
int getEmptyBlock(cfs_info*);
int get_parent(cfs_info*,char*,char*);
void replaceEntity(cfs_info*,int,int);
void append_file(cfs_info*,int,int);
int getDirEntities(cfs_info*,char*,string_List**);
void print_data(cfs_info*,char*);
void cleanSlashes(char**);
