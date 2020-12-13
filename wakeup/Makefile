obj-m   += wakeup.o 

KDIR    := /home/jeff/nvme/git/linux-test
PWD     := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) ccflags-y+="-g" modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f modules.order

