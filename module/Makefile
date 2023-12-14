obj-m = hw2.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CFLAGS_hw2.o  :=  -DDEBUG

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod *.mod.* .*.cmd *.o *.symvers *.order


