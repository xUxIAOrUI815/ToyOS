# Makefile
TARGET := riscv64-unknown-elf
CC := $(TARGET)-gcc
LD := $(TARGET)-ld
OBJCOPY := $(TARGET)-objcopy

CFLAGS := -Wall -O2 -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany
USER_CFLAGS := $(CFLAGS) -fno-stack-protector

QEMU_OPTS := -machine virt -nographic -bios default -kernel kernel.elf

# 1. 加入了 printf.c
# 2. trap.S 改名为 trap_entry.S (防止和 trap.c 冲突)
# 3. 加入了 trap.c
# KERNEL_SRCS := os/entry.S os/main.c os/sbi.c os/printf.c os/link_app.S os/trap/trap_entry.S os/trap/trap.c os/switch.S os/task.c
KERNEL_SRCS := os/entry.S os/main.c os/sbi.c os/printf.c os/link_app.S \
               os/trap/trap_entry.S os/trap/trap.c \
               os/switch.S os/task.c \
               os/mm.c os/paging.c
# 处理成 .o 文件列表
KERNEL_OBJS := $(KERNEL_SRCS:.c=.o)
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

USER_SRCS := user/entry.S user/app.c
USER_OBJS := $(USER_SRCS:.c=.o)
USER_OBJS := $(USER_OBJS:.S=.o)

all: run

# 编译 User App
user/app.bin: $(USER_OBJS) user/linker.ld
	$(LD) -T user/linker.ld -o user/app.elf $(USER_OBJS)
	$(OBJCOPY) -O binary user/app.elf user/app.bin

# 编译 Kernel
kernel.bin: user/app.bin $(KERNEL_OBJS) os/kernel.ld
	$(LD) -T os/kernel.ld -o kernel.elf $(KERNEL_OBJS)
	$(OBJCOPY) -O binary kernel.elf kernel.bin

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 运行
run: kernel.bin
	@echo "------------------------------------------------"
	@echo "ToyOS Phase 3: Trap & Syscall"
	@echo "------------------------------------------------"
	qemu-system-riscv64 $(QEMU_OPTS)

clean:
	rm -f os/*.o os/trap/*.o user/*.o *.elf *.bin user/*.bin user/*.elf