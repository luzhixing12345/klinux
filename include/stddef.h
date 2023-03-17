/*
 *Copyright (c) 2023 All rights reserved
 *@description: 标准定义头文件 定义了NULL 和 offsetof
 *@author: Zhixing Lu
 *@date: 2023-03-17
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
*/


#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#undef NULL
#define NULL ((void *)0)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif
