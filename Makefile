CC := x86_64-elf-gcc
AS := nasm

CFLAGS := -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-pic -fno-pie -mno-red-zone -mcmodel=kernel \
          -mno-80387 -mno-mmx -mno-sse -mno-sse2 \
          -Wall -Wextra -std=gnu11 -O2 -g -Ikernel

ASFLAGS := -f elf64

LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -Wl,-T,boot/linker.ld

LIMINE_DIR := third_party/limine

KERNEL   := build/kernel.elf
ISO      := diyaos.iso
ISO_ROOT := build/iso_root

OBJS := build/kmain.o build/serial.o build/gdt.o build/gdt_flush.o build/idt.o build/idt_flush.o build/isr.o build/isr_stubs.o build/pic.o build/timer.o build/irq.o build/irq_stubs.o

.PHONY: all iso run clean

all: $(ISO)

build/kmain.o: kernel/kmain.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/serial.o: kernel/drivers/serial.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/gdt.o: kernel/arch/x86_64/gdt.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/gdt_flush.o: kernel/arch/x86_64/gdt_flush.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJS) boot/linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $(KERNEL)
build/idt.o: kernel/arch/x86_64/idt.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/idt_flush.o: kernel/arch/x86_64/idt_flush.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

build/isr.o: kernel/arch/x86_64/isr.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/isr_stubs.o: kernel/arch/x86_64/isr_stubs.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

build/pic.o: kernel/drivers/pic.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/timer.o: kernel/drivers/timer.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/irq.o: kernel/arch/x86_64/irq.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/irq_stubs.o: kernel/arch/x86_64/irq_stubs.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

iso: $(KERNEL)
	@rm -rf $(ISO_ROOT)
	@mkdir -p $(ISO_ROOT)/boot/limine
	@mkdir -p $(ISO_ROOT)/EFI/BOOT
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp boot/limine.conf $(ISO_ROOT)/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $(ISO)
	$(LIMINE_DIR)/limine bios-install $(ISO)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf build $(ISO)