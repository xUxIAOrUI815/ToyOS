// // os/main.c
// // 声明我们在 sbi.c 里写的函数
// void console_putstr(char *str);

// // 简易的 printf 实现（为了简化，这里暂时直接用 putstr）
// // 后面章节我们会完善它
// void printf(char *str) {
//     console_putstr(str);
// }

// void main() {
//     // 打印第一行欢迎语
//     printf("\n");
//     printf("Hello, Student! ToyOS is running!\n");
//     printf("Phase 1: LibOS Initialized.\n");

//     // 死循环，防止 OS 退出
//     while (1) {};
// }
// os/main.c
typedef unsigned long long uint64;

// 引用 sbi.c 中的函数
void console_putstr(char *str);

void printf(char *str){
    console_putstr(str);
}

// 引用 link_app.S 里的符号，知道 App 在哪里
extern uint64 _app_start;
extern uint64 _app_end;

// App 应该被放到的目标地址
#define APP_BASE_ADDRESS 0x80400000

// 简单的 memcpy
void memcpy(void *dst, void *src, uint64 len) {
    char *d = (char *)dst;
    char *s = (char *)src;
    while (len--) {
        *d++ = *s++;
    }
}

void load_and_run_app() {
    uint64 *src = &_app_start;
    uint64 *dst = (uint64 *) APP_BASE_ADDRESS;
    uint64 *end = &_app_end;

    printf("Kernel: Loading app to 0x80400000...\n");

    // 1. 以 8 字节为单位 搬运内存
    while(src < end){
        *dst++ = *src++;
    }

    printf("Kernel: App Loaded! Jumping to App ... \n");

    // 2. 准备跳转
    // 定义函数指针，指向 APP_BASE_ADDRESS
    void (*app_entry)() = (void (*)()) APP_BASE_ADDRESS;

    // 3. 跳转
    app_entry();

    // 如果 App 死循环，则这一行不会被打印
    printf("Kernel: Error! App returned!\n");
}

int main() {
    printf("\n[ToyOS] Phase 2: Batch System\n");
    load_and_run_app();
    
    // 死循环
    while (1) {};
}