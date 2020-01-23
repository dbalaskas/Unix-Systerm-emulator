/* FILE: cfs_functions.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mds.h"
#include "disk.h"
// #include "cfs_file.h"

typedef enum {false, true} 							bool;
typedef enum {CRE, ACC, MOD} 						touch_mode;
typedef enum {LS_A, LS_R, LS_L, LS_U, LS_D, LS_H} 	ls_mode;
typedef enum {CP_I, CP_R, CP_L}			 			cp_mode;
typedef enum {RM_I, RM_R}		 					rm_mode;	

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

void update_superBlock(int);
int getTableSpace();
int traverse_cfs(int, char*,int);
int get_parent(int,char*,char*);
int getEmptyBlock();

