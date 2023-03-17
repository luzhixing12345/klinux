/*
 *Copyright (c) 2023 All rights reserved
 *@description: 系统名称结构头文件
 *@author: Zhixing Lu
 *@date: 2023-03-17
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
*/


#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/types.h>

struct utsname {
	char sysname[9];
	char nodename[9];
	char release[9];
	char version[9];
	char machine[9];
};

extern int uname(struct utsname * utsbuf);

#endif
