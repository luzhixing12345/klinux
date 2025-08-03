# 设置编译器
CC = gcc

# 设置编译选项
CFLAGS = -Wall -g

# 获取当前目录下所有.c文件的名称(不带.c扩展名)
SRCS := $(patsubst %.c,%,$(wildcard *.c))

# 默认目标:编译所有.c文件
all: $(SRCS)

# 编译规则:将.c文件编译成同名的可执行文件
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# 清理目标:删除所有生成的可执行文件
clean:
	rm -f $(SRCS)

# 防止make将clean当作文件名
.PHONY: all clean