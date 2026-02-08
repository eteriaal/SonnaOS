CC = clang
LD = ld.lld

CFLAGS = -target x86_64-unknown-elf \
         -ffreestanding -fno-pic -fno-pie -mno-red-zone \
         -fno-stack-protector -fshort-wchar -Wall -O2 \
		 -Ikernel -Ikernel/libkernel/include \
		 -MMD -MP -mcmodel=kernel -fno-omit-frame-pointer \
		 -mno-sse -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f

LDFLAGS = -T x86-64.lds -nostdlib -g

LIMINE_DIR ?= ./limine
QEMU ?= qemu-system-x86_64
QEMU_FLAGS ?= -M q35 -m 2G -serial stdio -display gtk \
			-no-reboot -no-shutdown

OVMF_CODE ?= $(firstword \
    $(wildcard /usr/share/OVMF/OVMF_CODE_4M.fd) \
    $(wildcard /usr/share/OVMF/OVMF_CODE.fd) \
	$(wildcard /usr/share/OVMF/x64/OVMF_CODE.4m.fd) \
    $(wildcard /usr/share/edk2/ovmf/OVMF_CODE.fd) \
    $(wildcard /usr/share/edk2-ovmf/x64/OVMF_CODE.fd) \
    $(wildcard /usr/share/qemu/ovmf-x86_64-code.fd) \
    $(wildcard /opt/homebrew/share/qemu/edk2-x86_64-code.fd) \
    $(wildcard /usr/local/share/qemu/OVMF_CODE.fd) \
)

ifeq ($(OVMF_CODE),)
$(error OVMF not found.)
endif


BUILD_DIR = build
SRC_DIR = kernel
ESP_DIR = $(BUILD_DIR)/esp

SPLEEN_URL ?= https://github.com/fcambus/spleen/releases/download/2.2.0/spleen-2.2.0.tar.gz
SPLEEN_TAR ?= spleen-2.2.0.tar.gz
SPLEEN_DIR ?= spleen

TARGET = $(BUILD_DIR)/estella.elf

C_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
DEPENDS := $(OBJECTS:.o=.d)

all: limine spleen $(TARGET)

limine:
	if [ ! -d "$(LIMINE_DIR)" ]; then \
		git clone --branch=v10.6.6-binary --depth=1 https://github.com/limine-bootloader/limine.git $(LIMINE_DIR); \
	fi

spleen:
	mkdir -p $(SPLEEN_DIR)
	if command -v curl >/dev/null 2>&1; then \
		curl -L -o $(SPLEEN_TAR) "$(SPLEEN_URL)"; \
	elif command -v wget >/dev/null 2>&1; then \
		wget -O $(SPLEEN_TAR) "$(SPLEEN_URL)"; \
	else \
		echo "||| failed to get spleen fonts" \
		exit 1; \
	fi 
	tar -xzf $(SPLEEN_TAR) \
		--strip-components=1 \
		-C $(SPLEEN_DIR) \
		--wildcards '*.psfu' || true
	rm -f $(SPLEEN_TAR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) x86-64.lds
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

esp: limine spleen $(TARGET) limine.conf
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot/limine
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
	cp $(TARGET) $(ESP_DIR)/boot/estella.elf
	cp limine.conf $(ESP_DIR)/boot/limine/limine.conf
	cp -r $(SPLEEN_DIR) $(ESP_DIR)/boot/$(SPLEEN_DIR)

run: esp
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
		-drive format=raw,file=fat:rw:$(ESP_DIR)

clean:
	rm -rf $(BUILD_DIR) 

distclean: clean 
	rm -rf $(LIMINE_DIR) $(SPLEEN_DIR)

compdb:
	bear -- make clean all

.PHONY: all esp run clean distclean

-include $(DEPENDS)