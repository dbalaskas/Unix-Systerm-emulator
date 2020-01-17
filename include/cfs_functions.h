/* FILE: cfs_functionss.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mds.h"
#include "disk.h"

typedef enum {false, true} bool;
typedef enum {CRE, ACC, MOD} touch_mode;

#define CALL(call,error,errString,errNum,returnVal)	\
returnVal = call;					\
if(returnVal == error)					\
{							\
	if(errString != NULL)				\
		perror(errString);			\
	exit(errNum);					\
}

void update_superBlock(int);
Location getLocation(int,int,int);
Location traverse_cfs(int,char*,unsigned int,unsigned int);

