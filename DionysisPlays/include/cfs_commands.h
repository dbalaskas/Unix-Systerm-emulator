/* FILE: cfs_commands.h */

#include "cfs_functions.h"

int cfs_workwith(cfs_info*);
int cfs_mkdir(cfs_info*,char*);
int cfs_touch(cfs_info*,char*,touch_mode);
int cfs_ln(cfs_info*,char*,char*);
int cfs_create(char*,int,int,unsigned int,int);
bool cfs_pwd();
bool cfs_cd(cfs_info*,char*);
bool cfs_ls(cfs_info*,bool*,char*);
bool cfs_cp(cfs_info*,bool*,string_List*,char*);
bool cfs_mv(cfs_info*,bool*,string_List*,char*);
bool cfs_cat(cfs_info*,string_List*,char*);
bool cfs_rm(cfs_info*,bool*,char*);
bool cfs_import(cfs_info*,string_List*,char*);
bool cfs_close(cfs_info*);

