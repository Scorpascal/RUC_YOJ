#include <stdio.h>   // 引入标准输入输出函数（printf, scanf）
#include <stdlib.h>  // 引入标准库（此处未使用动态内存，但保留）
#define MAXN 1000005  // 定义数组最大容量，略大于题目最大 n

int a[MAXN], b[MAXN]; // 全局数组：a 存原始/中间结果，b 临时存放按位排序结果
int cnt[10];         // 计数数组，记录每一位 0-9 出现次数

int main() {
    int n;
    scanf("%d", &n);              // 读入要排序的数的个数 n
    for(int i=0; i<n; i++) scanf("%d", &a[i]); // 依次读入 n 个正整数到数组 a

    int max = a[0];               // 初始化 max 为第一个元素，用于找到最大值决定位数
    for(int i=1; i<n; i++)        // 遍历剩余元素
        if(a[i] > max) max = a[i]; // 更新最大值

    // 基数排序，按位处理（从低位到高位）
    for(long long exp=1; max/exp>0; exp*=10) { // exp 表示当前处理的位（1,10,100,...）
        for(int i=0; i<10; i++) cnt[i]=0;      // 清零计数数组

        for(int i=0; i<n; i++)                 // 统计每个数字在当前位上的数字出现次数
            cnt[(a[i]/exp)%10]++;

        for(int i=1; i<10; i++) cnt[i] += cnt[i-1]; // 将计数转换为位置（前缀和），用于稳定排序

        for(int i=n-1; i>=0; i--) {            // 从后向前遍历以保持稳定性
            int d = (a[i]/exp)%10;             // 取出当前位的数字
            b[--cnt[d]] = a[i];                // 将元素放到 b 中对应的位置，计数减一
        }

        for(int i=0; i<n; i++) a[i]=b[i];      // 将本轮排序结果复制回 a，为下一位排序做准备
    }

    for(int i=0; i<n; i++) printf("%d\n", a[i]); // 输出排序后的每个数，每行一个
    return 0;                                     // 返回 0，程序正常结束
}