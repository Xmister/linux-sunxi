
################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################
PWD=$(shell pwd)

obj-m+=nand.o
nand-objs := nand_interface.o libnand

#install: build
#	cp nand.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(KERNEL_PATH)
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
