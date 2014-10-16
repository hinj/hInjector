obj-m:=hInjLKM.o
KDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules -lxenctrl
	$(CC) -o hInjSender hInjSender.c
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
