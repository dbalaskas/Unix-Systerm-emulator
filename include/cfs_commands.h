/* FILE: cfs_commands.h */

#include "cfs_functions.h"

int cfs_workwith(char*);
bool cfs_touch(int,char*,touch_mode);
int cfs_create(char*,int,int,int,int);
bool cfs_pwd();
bool cfs_cd(int, char*);
bool cfs_ls(int, char*, char*);
bool cfs_close(int);

