#
# Makefile for the kernel USB device drivers.
#

# Object files in subdirectories
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
obj-y := lgit_multi_rmnet.o
lgit_multi_rmnet-objs := qmi_ctl.o qmi_svc.o util.o rmnet.o

all: clean build

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

