/* FILE: cfs_commands.h */

#include "cfs_functions.h"

int cfs_workwith(char*);
int cfs_mkdir(int,char*);
int cfs_touch(int,char*,touch_mode);
int cfs_create(char*,int,int,unsigned int,int);
bool cfs_pwd();
bool cfs_cd(int, char*);
bool cfs_ls(int, bool*, char*);
bool cfs_cat(int,string_List*,char*);
bool cfs_rm(int,bool*,char*);
bool cfs_import(int,string_List*,char*);
bool cfs_close(int);

