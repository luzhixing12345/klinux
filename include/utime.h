/*
 *Copyright (c) 2023 All rights reserved
 *@description: 用户时间头文件 定义了访问和修改时间结构以及utime原型
 *@author: Zhixing Lu
 *@date: 2023-03-17
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
*/


#ifndef _UTIME_H
#define _UTIME_H

#include <sys/types.h>	/* I know - shouldn't do this, but .. */

struct utimbuf {
	time_t actime;
	time_t modtime;
};

extern int utime(const char *filename, struct utimbuf *times);

#endif
