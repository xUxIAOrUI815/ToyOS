# os/link_app.S
.align 4
.section .data
.global _app_start
.global _app_end

_app_start:
    # 魔法操作：直接把二进制文件粘贴到这里
    .incbin "/home/xuxiaorui/os-project/user/app.bin"
_app_end: