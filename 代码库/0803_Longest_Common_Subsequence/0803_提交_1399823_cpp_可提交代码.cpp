/*
  程序说明（整体原理）：
  - 目标：求两条 DNA 序列的最长公共子序列（LCS，Longest Common Subsequence）的长度。
  - 经典动态规划：设 dp[i][j] 为 a 的前 i 个字符与 b 的前 j 个字符的 LCS 长度，
      当 a[i-1] == b[j-1] 时：dp[i][j] = dp[i-1][j-1] + 1
      否则：           dp[i][j] = max(dp[i-1][j], dp[i][j-1])
  - 由于 dp[i][j] 只依赖于上一行（i-1 行）与当前行左边（j-1），可以用两行一维数组滚动替代二维数组，
    将空间从 O(n*m) 优化到 O(m)（m 为第二条序列长度）。
  - 时间复杂度：O(n*m)，n、m ≤ 1000 时在题目限制内可行。
  - 空间复杂度：O(m)（双数组，长度 m+1）。
*/

#include <stdio.h>   // 提供 printf、scanf 等输入输出函数
#include <string.h>  // 提供 strlen 等字符串函数
#include <stdlib.h>  // 提供 calloc、free 等内存管理函数

int main(void) {
    /* 存放输入的两条序列。
       用静态数组避免栈溢出并且大小固定（题目保证长度 ≤ 1000），留出 1005 保证安全。 */
    static char a[1005], b[1005];

    /* 读取两行输入，%1004s 限制读取长度防止越界。
       scanf 返回成功读取的项数，若失败（比如 EOF）则直接退出程序。 */
    if (scanf("%1004s", a) != 1) return 0;
    if (scanf("%1004s", b) != 1) return 0;

    /* 计算两条序列实际长度 */
    int n = (int)strlen(a), m = (int)strlen(b);

    /* 若任一序列为空，LCS 长度显然为 0，直接输出并退出 */
    if (n == 0 || m == 0) {
        printf("0\n");
        return 0;
    }

    /* 分配两行一维 DP 数组，长度为 m+1（包含 j=0 的基线）。
       使用 calloc 初始化为 0，这样不需要额外赋初值。 */
    int *prev = (int*)calloc(m + 1, sizeof(int)); // 表示 dp[i-1][*]
    int *cur  = (int*)calloc(m + 1, sizeof(int)); // 表示 dp[i][*]
    if (!prev || !cur) return 0; // 分配失败则退出（极少发生）

    /* 外层遍历 a 的每个字符（i 从 1 到 n），内层遍历 b 的每个字符（j 从 1 到 m）。
       使用 1-based 的 i,j 可直接对应 dp 的定义（dp[i][j] 对应 a 前 i、b 前 j）。 */
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            /* 核心转移：
               - 若当前字符相等（a[i-1] == b[j-1]），说明可在 dp[i-1][j-1] 的基础上扩展 1；
               - 否则取来自上方（prev[j] 即 dp[i-1][j]）或左方（cur[j-1] 即 dp[i][j-1]）的较大值。 */
            if (a[i - 1] == b[j - 1]) cur[j] = prev[j - 1] + 1;
            else cur[j] = (prev[j] > cur[j - 1]) ? prev[j] : cur[j - 1];
        }
        /* 当前行计算完成后，将 cur 设为下一次的 prev，prev 设为 cur 的旧数组。
           通过交换指针而不是复制数组实现 O(1) 的行切换。下一次循环会覆盖 cur（旧 prev）的内容。 */
        int *tmp = prev; prev = cur; cur = tmp;
    }

    /* 循环结束后，结果保存在 prev[m]（因为最后一次交换后 prev 指向刚计算完成的那一行）。 */
    printf("%d\n", prev[m]);

    /* 释放动态分配的内存。注意：此时 cur 指向原先的 prev 的缓冲区（已交换），也需释放。 */
    free(prev);
    free(cur);

    return 0;
}