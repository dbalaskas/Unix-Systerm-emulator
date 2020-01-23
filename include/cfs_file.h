#include "mds.h"
#include "disk.h"

typedef struct {
    superBlock	sB;
    char        fileName;
    int         fileDesc;
    int		    cfs_current_nodeid;
    char		*inodeTable;
    int		    inodeSize;
    List		*holes;
} cfs_info;
