obj-m := sensor.o
sensor-objs := sensormodule.o utils.o
KDIR := /home/luisbatista/buildroot/buildroot-2024.08/output/build/linux-custom
ARCH ?= arm64
#CROSS_COMPILE ?= ${GCC_CROSS}
CROSS_COMPILE ?= /home/luisbatista/buildroot/buildroot-2024.08/output/host/bin/aarch64-buildroot-linux-gnu-

export ARCH CROSS_COMPILE


KBUILD_TARGETS	:= clean help modules modules_install

.PHONY: all $(KBUILD_TARGETS)

all: modules
$(KBUILD_TARGETS):
	$(MAKE) -C ${KDIR} M="$(shell pwd)" $@
