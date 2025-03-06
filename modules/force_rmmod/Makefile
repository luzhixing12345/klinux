MODULE_NAME := force_rmmod
EXTRA_CFLAGS  += -g -std=gnu99  -Wfatal-errors 

obj-m := force_rmmod.o

CURRENT_PATH := $(shell pwd)
KERNEL_PATH := $(shell dirname $(shell dirname $(CURRENT_PATH)))

modules:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) modules


clean:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) clean
	rm -f modules.order Module.symvers Module.markers

.PHNOY:modules modules_install clean
