DEV	:=	push_switch
DD	:=	fpga_$(DEV)_driver
APP	:=	lcd_touch_circle
TFTPDIR	:=	/nfsroot
CC	=	arm-none-linux-gnueabi-gcc
KDIR	:=	/root/work/achroimx6q/kernel
PWD	:=	$(shell pwd)
obj-m	:=	$(DD).o

all:	$(DD)	$(APP)

$(DD):	$(DD).c
	$(MAKE)	-C	$(KDIR)	SUBDIRS=$(PWD)	modules	ARCH=arm

$(APP):	$(APP).c
	$(CC)	-o	$(APP)	$(APP).c

install:
	cp	-a	$(DD).ko	$(APP)	$(TFTPDIR)

clean:
	rm	-rf	*.ko	*.mod.*	*.order	*.symvers	*.o	$(APP)

new:
	$(MAKE)	clean
	$(MAKE)

