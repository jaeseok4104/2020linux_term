#Makefile for a basic kernel module
obj-m := fpga_fnd_driver.o

KDIR := /work/achroimx6q/kernel
PWD := $(shell pwd)

all: driver app
driver:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm
app:
	arm-none-linux-gnueabi-gcc -static -o 0525_1 0525_1.c
install:
	cp -a fpga_fnd_driver.ko /nfsroot
	cp -a 0525_1 /nfsroot
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf fpga_test_fnd
	rm -rf Module.symvers
	rm -rf modules.order
	rm -rf .fnd*
	rm -rf .tmp*
