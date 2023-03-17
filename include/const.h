/*
 *Copyright (c) 2023 All rights reserved
 *@description: 常数符号头文件, 目前仅定义了i节点中i_mode字段的各标志位
 *@author: Zhixing Lu
 *@date: 2023-03-17
 *@email: luzhixing12345@163.com
 *@Github: luzhixing12345
*/


#ifndef _CONST_H
#define _CONST_H

#define BUFFER_END 0x200000

#define I_TYPE          0170000
#define I_DIRECTORY	0040000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000
#define I_SET_UID_BIT   0004000
#define I_SET_GID_BIT   0002000

#endif
