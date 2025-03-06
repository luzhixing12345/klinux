
obj-m := kvm_ept_sample.o
kvm_ept_sample-objs := main.o interact.o sampler.o queue.o

CURRENT_PATH := $(shell pwd)
KERNEL_PATH := $(shell dirname $(shell dirname $(CURRENT_PATH)))

modules:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) clean