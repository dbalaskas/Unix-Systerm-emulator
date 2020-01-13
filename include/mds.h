/* FILE: mds.h */

#include <time.h>

typedef struct {
        unsigned int nodeid;
//      char *filename;
        unsigned int size;
        unsigned int type;
        unsigned int parent_nodeid;
        time_t creation_time;
        time_t access_time;
        time_t modification_time;
//      Datastream data;
} MDS;
/*
typedef struct {
        unsigned int datablocks[DATABLOCK_NUM];
} Datastream;
*/

/*
 *
 * METADATA of each file is saved like that: filename(maxFileNameSize) + MDS + datablocks(maxDataBlocksNum)
 *
 * */
