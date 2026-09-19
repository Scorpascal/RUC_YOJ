#include <stdio.h>

int F(int n) {
    

//煞笔nc
    if (n <= 0) return 0;
    if (n <= 2) return 1;
    
    long long result[2][2] = {{1, 0}, {0, 1}}; // 单位矩阵
    long long base[2][2] = {{1, 1}, {1, 0}};   // 基础矩阵
    
    n -= 2; // 因为前两项已知，我们需要计算 base^(n-2) 乘以初始向量
    
    while (n > 0) {
        if (n & 1) {
            long long temp[2][2] = {0};
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 0; k < 2; k++) {
                        temp[i][j] += result[i][k] * base[k][j];
                    }
                }
            }
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++)
                    result[i][j] = temp[i][j];
        }
        
        long long temp[2][2] = {0};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    temp[i][j] += base[i][k] * base[k][j];
                }
            }
        }
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++)
                base[i][j] = temp[i][j];
                
        n >>= 1;
    }
    
    // 最终结果是 result * [F(2), F(1)]^T = result * [1, 1]^T
    // 我们需要的是第一行第一列 * [1, 1]^T 的结果
    return result[0][0] + result[0][1];

}

int main() {
    int n;
    scanf("%d", &n);
    
    printf("%d\n", F(n));
    return 0;
}