/* FILE: cfs_commands.h */

#include "mds.h"
#include "disk.h"


#define CALL(call,error,errString,errNum)       \
if(call == error)                               \
{                                               \
        perror(errString);                      \
        exit(errNum);                           \
}


/*
1. cfs workwith <FILE>
2. cfs mkdir <DIRECTORIES>
3. cfs touch <OPTIONS> <FILES>
4. cfs pwd
5. cfs cd <PATH>
6. cfs ls <OPTIONS> <FILES>
7. cfs cp <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>
8. cfs cat <SOURCE FILES> -o <OUTPUT FILE>
9. cfs ln <SOURCE FILES> <OUTPUT FILE>
10. cfs mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY>
11. cfs rm <OPTIONS> <DESTINATIONS>
12. cfs import <SOURCES> ... <DIRECTORY>
13. cfs export <SOURCES> ... <DIRECTORY>
14. cfs create <OPTIONS> <FILE>. Δημιουργία ενός cfs στο αρχείο <FILE>.
*/

void cfs_create(char*,int*,int*,int*,int*);
