OPENWRT_DIR=/home/prog/openwrt/2023-openwrt/openwrt-2023
PREFIX=arm
export STAGING_DIR=$(OPENWRT_DIR)/staging_dir/toolchain-$(PREFIX)_cortex-a7+neon-vfpv4_gcc-11.2.0_musl_eabi
TARGET_DIR=$(OPENWRT_DIR)/build_dir/target-arm_cortex-a7+neon-vfpv4_musl_eabi
GCC=$(STAGING_DIR)/bin/$(PREFIX)-openwrt-linux-gcc
LD=$(STAGING_DIR)/bin/$(PREFIX)-openwrt-linux-ld
OD=$(STAGING_DIR)/bin/$(PREFIX)-openwrt-linux-objdump
OC=$(STAGING_DIR)/bin/$(PREFIX)-openwrt-linux-objcopy

#так же поправь ссылки в ./libs и ./include на актуальну юверсию openwrt
CC       = $(GCC)
LIBS     = -L./libs -lncursesw -lreadline -lhistory
CFLAGS   = -O2 -Wno-unused-result -Wunused -I./include
OBJS     = hdlc.o qcio.o memio.o chipconfig.o utils.o
BINS     = qcommand qrmem qrflash qdload mibibsplit qwflash qwdirect qefs qnvram qblinfo qident qterminal qbadblock qflashparm efs2_imei_patcher

.PHONY: all clean

all: $(BINS)

install:
	rm -Rf ./res
	mkdir ./res
	cp $(BINS) ./res/
	cp chipset.cfg dload.sh ./res/
	mkdir ./res/loaders
	cp ./loaders/*9x55* ./res/loaders

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BINS): %: %.o $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f *.o
	rm -f $(BINS)

chipconfig : utils.o
qefs : efsio.o
qdload : sahara.o
qdload qrflash qwdirect qbadblock : ptable.o
$(addsuffix .o, $(BINS)) $(OBJS) : include.h
