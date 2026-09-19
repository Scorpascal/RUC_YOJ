#include <stdio.h>      // 包含标准输入输出函数声明（printf, scanf 等）
#include <stdlib.h>     // 包含通用工具函数（malloc, free, rand, srand, exit 等）
#include <time.h>       // 包含时间相关函数（time，用于生成随机种子）

// 快速排序函数：对 arr 数组在索引区间 [left, right] 进行就地排序
void quicksort(int arr[], int left, int right) {
    if (left >= right) return;                // 若区间长度小于等于1，直接返回（无需排序）
    // 随机选取基准
    int pivot_idx = left + rand() % (right - left + 1); // 在 left..right 范围内随机选取一个索引作为基准
    int pivot = arr[pivot_idx];               // 保存基准值
    arr[pivot_idx] = arr[left];               // 将基准所在元素和左端元素交换（把基准临时移动到左端）
    arr[left] = pivot;                        // 完成基准与左端元素的交换

    int i = left, j = right;                  // i 从左向右扫描，j 从右向左扫描
    while (i < j) {                           // 当 i 和 j 未相遇时持续移动并交换元素
        while (i < j && arr[j] >= pivot) j--; // 从右侧找到第一个小于 pivot 的元素（或遇到 i 停止）
        if (i < j) arr[i++] = arr[j];         // 将该较小元素移到左侧位置，并将 i 右移一位
        while (i < j && arr[i] <= pivot) i++; // 从左侧找到第一个大于 pivot 的元素（或遇到 j 停止）
        if (i < j) arr[j--] = arr[i];         // 将该较大元素移到右侧位置，并将 j 左移一位
    }
    arr[i] = pivot;                           // 将基准值放回 i 位置（此时 i 为基准最终位置）
    quicksort(arr, left, i - 1);              // 递归排序基准左侧子区间
    quicksort(arr, i + 1, right);             // 递归排序基准右侧子区间
}

int main() {
    int n;
    scanf("%d", &n);                          // 从标准输入读取一个整数 n（数组元素个数）
    int arr[n];                               // 在栈上分配长度为 n 的整型数组（变长数组，C99 支持）
    for (int i = 0; i < n; i++) {             // 循环读取 n 个整数到数组中
        scanf("%d", &arr[i]);                 // 从标准输入读取第 i 个整数并存入 arr[i]
    }
    srand(time(0));                           // 用当前时间初始化随机数种子，确保每次运行 rand() 不同
    quicksort(arr, 0, n - 1);                 // 调用 quicksort 对整个数组进行排序（索引 0 到 n-1）
    for (int i = 0; i < n; i++) {             // 输出排序后的数组
        printf("%d\n", arr[i]);               // 每行打印一个元素，末尾换行
    }
    return 0;                                 // 返回 0 给操作系统，表示程序正常结束
}