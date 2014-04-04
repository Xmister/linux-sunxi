PWD=$(shell pwd)

MALI_DRV_ROOT=DX910-SW-99002-r3p1-01rel1/driver/src/devicedrv/mali
MALI_UMP_ROOT=DX910-SW-99002-r3p1-01rel1/driver/src/devicedrv/ump

#MALI_DRV_ROOT=DX910-BU-00000-r2p4-02rel1/devicedrv/mali
#MALI_UMP_ROOT=DX910-BU-00000-r2p4-02rel1/devicedrv/ump

#MALI_DRV_ROOT=DX910-SW-99002-r3p0-04rel0/driver/src/devicedrv/mali
#MALI_UMP_ROOT=DX910-SW-99002-r3p0-04rel0/driver/src/devicedrv/ump

#MALI_DRV_ROOT=DX910-SW-99002-r3p1-00dev0/driver/src/devicedrv/mali
#MALI_UMP_ROOT=DX910-SW-99002-r3p1-00dev0/driver/src/devicedrv/ump


#nstall: build
#	cp $(MALI_DRV_ROOT)/mali.ko $(LICHEE_MOD_DIR)/
#	cp $(MALI_UMP_ROOT)/ump.ko $(LICHEE_MOD_DIR)/
	

build:
	$(MAKE) -C $(MALI_UMP_ROOT) CONFIG=ca8-virtex820-m400-1 BUILD=release KDIR=${KERNEL_PATH}
	$(MAKE) -C $(MALI_DRV_ROOT) USING_MMU=1 USING_UMP=1 USING_PMM=1 BUILD=release \
		CONFIG=ca8-virtex820-m400-1 KDIR=${KERNEL_PATH}



clean:
	$(MAKE) -C $(MALI_UMP_ROOT) CONFIG=ca8-virtex820-m400-1 BUILD=release KDIR=${KERNEL_PATH} clean
	$(MAKE) -C $(MALI_DRV_ROOT) USING_MMU=1 USING_UMP=1 USING_PMM=1 BUILD=release \
		CONFIG=ca8-virtex820-m400-1 KDIR=${KERNEL_PATH} clean

