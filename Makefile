#Specifies type of EP
P4080_EP=n
C293_EP=y

#Controls the debug print level
DEBUG_PRINT=n
ERROR_PRINT=n
INFO_PRINT=n

#Enable HASH/SYMMETRIC offloading
CONFIG_FSL_C2X0_HASH_OFFLOAD=n
CONFIG_FSL_C2X0_HMAC_OFFLOAD=n
CONFIG_FSL_C2X0_SYMMETRIC_OFFLOAD=n

#Enable RNG offloading
RNG_OFFLOAD=n

#Specifies whether SEC DMA support to be enabled /disabled in the driver
#If enabled, then Host DMA support would be always disabled.
USE_SEC_DMA=y

#Specifies whether host DMA support to be enabled /disabled in the driver
USE_HOST_DMA=n

#Specifies whether driver/firmware is running high performance mode
HIGH_PERF_MODE=y

#Enhance pkc kernel test performance, disable kernel test schedule and
#restriction number of c29x_fw enqueue and dequeue crypto
ENHANCE_KERNEL_TEST=n

#Specify building host-driver to support Virtualization
VIRTIO_C2X0=n

KERNEL_DIR ?=/lib/modules/$(shell uname -r)/build
ifneq (${ARCH},)
KERNEL_MAKE_OPTS += ARCH=${ARCH}
endif
ifneq (${CROSS_COMPILE},)
KERNEL_MAKE_OPTS += CROSS_COMPILE=${CROSS_COMPILE}
endif

ifeq ($(HIGH_PERF_MODE),y)
EXTRA_CFLAGS += -DHIGH_PERF
endif

EXTRA_CFLAGS += -g3

ifeq ($(P4080_EP),y)
EXTRA_CFLAGS += -DP4080_EP
endif

ifeq ($(C293_EP),y)
EXTRA_CFLAGS += -DC293_EP
endif

ifeq ($(DEBUG_PRINT),y)
EXTRA_CFLAGS += -DDEV_PRINT_DBG -DPRINT_DEBUG
endif

ifeq ($(ERROR_PRINT),y)
EXTRA_CFLAGS += -DDEV_PRINT_ERR -DPRINT_ERROR
endif

ifeq ($(INFO_PRINT),y)
EXTRA_CFLAGS += -DPRINT_INFO
endif

EXTRA_CFLAGS += -DDEV_PHYS_ADDR_64BIT -DDEV_VIRT_ADDR_32BIT

ifeq ($(VIRTIO_C2X0),y)
EXTRA_CFLAGS += -DVIRTIO_C2X0
endif

ifeq ($(CONFIG_FSL_C2X0_HASH_OFFLOAD),y)
EXTRA_CFLAGS += -DHASH_OFFLOAD
ifeq ($(CONFIG_FSL_C2X0_HMAC_OFFLOAD),y)
EXTRA_CFLAGS += -DHMAC_OFFLOAD
endif
endif

ifeq ($(CONFIG_FSL_C2X0_SYMMETRIC_OFFLOAD),y)
EXTRA_CFLAGS += -DSYMMETRIC_OFFLOAD
endif

ifeq ($(RNG_OFFLOAD),y)
EXTRA_CFLAGS += -DRNG_OFFLOAD
endif

ifeq ($(USE_SEC_DMA), y)
USE_HOST_DMA = n
EXTRA_CFLAGS += -DSEC_DMA
endif

ifeq ($(USE_HOST_DMA), y)
EXTRA_CFLAGS += -DUSE_HOST_DMA
endif

ifeq ($(ENHANCE_KERNEL_TEST), y)
EXTRA_CFLAGS += -DENHANCE_KERNEL_TEST
endif

EXTRA_CFLAGS += -I$(src)/host_driver -I$(src)/algs -I$(src)/crypto_dev -I$(src)/dcl -I$(src)/test -g

DRIVER_KOBJ = fsl_pkc_crypto_offload_drv
RSA_TEST_KOBJ = "rsa_test"
DSA_TEST_KOBJ = "dsa_test"
ECDSA_TEST_KOBJ = "ecdsa_test"
DH_TEST_KOBJ = "dh_test"
ECDH_TEST_KOBJ = "ecdh_test"

CONFIG_FSL_C2X0_CRYPTO_DRV ?= m

obj-$(CONFIG_FSL_C2X0_CRYPTO_DRV) = $(DRIVER_KOBJ).o

$(DRIVER_KOBJ)-objs := host_driver/fsl_c2x0_driver.o
$(DRIVER_KOBJ)-objs += host_driver/fsl_c2x0_crypto_layer.o
$(DRIVER_KOBJ)-objs += host_driver/memmgr.o
$(DRIVER_KOBJ)-objs += host_driver/command.o
$(DRIVER_KOBJ)-objs += host_driver/sysfs.o
ifneq ("$(ARCH)","powerpc")
$(DRIVER_KOBJ)-objs += crypto/pkc.o
endif
$(DRIVER_KOBJ)-objs += host_driver/dma.o
$(DRIVER_KOBJ)-objs += algs/algs.o
$(DRIVER_KOBJ)-objs += algs/rsa.o
$(DRIVER_KOBJ)-objs += algs/dsa.o
$(DRIVER_KOBJ)-objs += algs/dh.o
$(DRIVER_KOBJ)-objs += algs/desc_cnstr.o
$(DRIVER_KOBJ)-objs += algs/rng_init.o
$(DRIVER_KOBJ)-objs += crypto_dev/algs_reg.o
ifeq ($(CONFIG_FSL_C2X0_HASH_OFFLOAD),y)
$(DRIVER_KOBJ)-objs += algs/hash.o
endif
ifeq ($(CONFIG_FSL_C2X0_SYMMETRIC_OFFLOAD),y)
$(DRIVER_KOBJ)-objs += algs/symmetric.o
endif
$(DRIVER_KOBJ)-objs += algs/rng.o

ifeq ($(VIRTIO_C2X0),n)
$(DRIVER_KOBJ)-objs += test/rsa_test.o
$(DRIVER_KOBJ)-objs += test/dsa_test.o
$(DRIVER_KOBJ)-objs += test/ecdsa_test.o
$(DRIVER_KOBJ)-objs += test/ecp_test.o
$(DRIVER_KOBJ)-objs += test/ecpbn_test.o
$(DRIVER_KOBJ)-objs += test/dh_test.o
$(DRIVER_KOBJ)-objs += test/ecdh_test.o
$(DRIVER_KOBJ)-objs += test/ecdh_keygen_test.o
$(DRIVER_KOBJ)-objs += test/test.o
endif

.PHONY: build

build:
	make -C $(KERNEL_DIR) SUBDIRS=`pwd` modules
	$(CROSS_COMPILE)gcc  -Wall apps/cli/cli.c -o apps/cli/cli -static

modules_install:
	make -C $(KERNEL_DIR) SUBDIRS=`pwd` modules_install

clean:
	make -C $(KERNEL_DIR) SUBDIRS=`pwd` clean

dist: clean
