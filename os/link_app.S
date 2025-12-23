# os/link_app.S
# 将二进制数据直接嵌入到内核镜像 kernel.bin 中
.align 4
.section .data
.global _app_start
.global _app_end

_app_start:
    # 把二进制文件粘贴到这里
    .incbin "/home/xuxiaorui/os-project/user/app.bin"
_app_end: