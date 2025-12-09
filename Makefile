# Makefile
TARGET := riscv64-unknown-elf
CC := $(TARGET)-gcc
LD := $(TARGET)-ld
OBJCOPY := $(TARGET)-objcopy

# 编译参数
CFLAGS := -Wall -O2 -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany
USER_CFLAGS := $(CFLAGS) -fno-stack-protector

# QEMU 参数
QEMU_OPTS := -machine virt -nographic -bios default -kernel kernel.elf

# 源文件定义
KERNEL_SRCS := os/entry.S os/main.c os/sbi.c os/link_app.S
KERNEL_OBJS := $(KERNEL_SRCS:.c=.o)
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

USER_SRCS := user/entry.S user/app.c
USER_OBJS := $(USER_SRCS:.c=.o)
USER_OBJS := $(USER_OBJS:.S=.o)

all: run

# ----------------------------------
# 下面的目标行 (冒号结尾) 必须顶格！
# 下面的命令行 (GCC/LD) 必须用 Tab 缩进！
# ----------------------------------

# 1. 编译 User App
user/app.bin: $(USER_OBJS) user/linker.ld
	$(LD) -T user/linker.ld -o user/app.elf $(USER_OBJS)
	$(OBJCOPY) -O binary user/app.elf user/app.bin

# 2. 编译 Kernel
kernel.bin: user/app.bin $(KERNEL_OBJS) os/kernel.ld
	$(LD) -T os/kernel.ld -o kernel.elf $(KERNEL_OBJS)
	$(OBJCOPY) -O binary kernel.elf kernel.bin

# 3. 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 4. 运行
run: kernel.bin
	@echo "------------------------------------------------"
	@echo "ToyOS Batch System is starting..."
	@echo "------------------------------------------------"
	qemu-system-riscv64 $(QEMU_OPTS)

clean:
	rm -f os/*.o user/*.o *.elf *.bin user/*.bin user/*.elf