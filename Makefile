ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: build clean

build:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	gcc ioctl_test.c -o ioctl_test
	gcc output_packets.c -o output_packets -lpcap

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf ioctl_test output_packets
else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
obj-m :=    khial.o
khial-objs := khial_main.o khial_ethtool.o
endif
