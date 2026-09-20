#include <stdio.h>

/*
正确判定与构造：
设基准表示：a31 = 1，其余 a_i = -1，则初始值 base = 1。
把某位 i (0<=i<=30) 从 -1 改成 0，值增加 2^i。
不再修改为 1（增加 2*2^i），只用 {-1,0,1} 中的 0 来累加，足够覆盖所有需要的增量形式：n = 1 + Σ 2^i。
因此 n 可分解 ⇔ n >= 1 且 (n-1) 的二进制表示中没有相邻的 1（否则需要两个相邻的 0，违反限制）。
令 N = n - 1。
若 (N & (N >> 1)) != 0 输出 NO；否则：
a31 = 1；
对 i=0..30：若 N 的第 i 位为 1 ⇒ a_i = 0；否则 a_i = -1。
该构造无相邻 0（因为 N 无相邻 1），满足条件。
注意：n=0 输出 NO。
*/

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        unsigned int n;
        if (scanf("%u", &n) != 1) return 0;
        
        // 如果 n 是 4 的倍数 (包括 0)，则必须 a0=0, a1=0，违反“无相邻0”的限制
        if (n % 4 == 0) {
            puts("NO");
            continue;
        }
        
        puts("YES");
        int a[32];
        long long rem = n; // 使用 long long 防止计算溢出，虽然逻辑上收敛很快
        
        for (int i = 0; i < 31; ++i) {
            if (rem % 2 == 0) {
                a[i] = 0;
                rem /= 2;
            } else {
                // 当前为奇数，选择 a[i] = 1 或 -1
                // 策略：选择使得下一位余数为奇数，从而避免下一位出现 0
                // 这样可以保证一旦进入非零状态，就不会再出现 0，避免相邻 0
                long long r1 = (rem - 1) / 2; // 尝试选 1
                // long long r2 = (rem + 1) / 2; // 尝试选 -1
                
                if (r1 % 2 != 0) {
                    a[i] = 1;
                    rem = r1;
                } else {
                    a[i] = -1;
                    rem = (rem + 1) / 2;
                }
            }
        }
        a[31] = (int)rem;
        
        // 输出
        for (int row = 0; row < 4; ++row) {
            int start = 31 - row * 8;
            for (int k = 0; k < 8; ++k) {
                if (k) putchar(' ');
                printf("%d", a[start - k]);
            }
            putchar('\n');
        }
    }
    return 0;
}