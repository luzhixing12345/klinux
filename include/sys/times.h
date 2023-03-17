/*
 *Copyright (c) 2023 All rights reserved
 *@description: 定义了进程中运行时间结构tms以及times函数原型
 *@author: Zhixing Lu
 *@date: 2023-03-17
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
*/


#ifndef _TIMES_H
#define _TIMES_H

#include <sys/types.h>

struct tms {
	time_t tms_utime;
	time_t tms_stime;
	time_t tms_cutime;
	time_t tms_cstime;
};

extern time_t times(struct tms * tp);

#endif
