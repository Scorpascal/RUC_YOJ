#include <stdio.h>      // 引入标准输入输出库
#include <string.h>     // 引入字符串处理库

int main(void) {
    char num[105];    // 定义字符数组，存储大整数（最多101位+结束符，预留空间）
    char line[128];   // 定义字符数组，存储第二行输入（hello world 变体）

    if (scanf("%102s", num) != 1) return 0; // 读入第一行大整数，若失败则退出
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {} // 清理输入缓冲区直到行尾，防止影响后续读取
    if (!fgets(line, sizeof(line), stdin)) return 0; // 读入第二行字符串，若失败则退出
    size_t len = strlen(line);                       // 获取第二行字符串长度
    if (len && line[len-1] == '\n') line[len-1] = '\0'; // 去除末尾换行符（如果有）

    puts(num);   // 输出第一行大整数
    puts(line);  // 输出第二行字符串
    return 0;    // 程序正常结束
}