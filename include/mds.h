/* FILE: mds.h */

#include <time.h>
typedef enum {File, Directory, Link} cfs_type;

typedef struct {
	unsigned int	nodeid;
	unsigned int	size;
	cfs_type	type;
	unsigned int	parent_nodeid;
	time_t		creation_time;
	time_t		access_time;
	time_t		modification_time;
	int		datablocksCounter;			// Number of elements in table datablocks (at any time)
} MDS;

typedef struct {
	unsigned int *datablocks;
} Datastream;

/*
 *
 * METADATA of each file is saved like that: filename(maxFileNameSize) + MDS + datablocks[datablocksCounter]
 *
 * */
