ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

KERN_SRC := /home/richard/work/2025/3dprinter/linux

obj-m += test_phy.o

all:
	$(MAKE) -C $(KERN_SRC) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERN_SRC) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
