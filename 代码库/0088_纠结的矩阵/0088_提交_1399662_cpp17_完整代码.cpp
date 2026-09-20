#include<stdio.h>
#include<stdlib.h>

int** f(int a) {
    int **t = (int**)malloc(a * sizeof(int*));
    for(int i = 0; i < a; i++) {
        t[i] = (int*)malloc(a * sizeof(int));
    }
    
    if(a == 1) {
        t[0][0] = 1;
        return t;
    }
    
    int b = (a-1)*(a-1) + 1;
    
    // 递归获取子矩阵并复制
    int **sub = f(a-1);
    for(int i = 0; i < a-1; i++) {
        for(int j = 0; j < a-1; j++) {
            t[i][j] = sub[i][j];
        }
        free(sub[i]);  // 释放子矩阵内存
    }
    free(sub);
    
    if(a % 2 == 0) {
        // 偶数情况：先填充右边，再填充底部
        int i = 0, j = a-1;
        // 填充右边列
        while(i < a) {
            t[i][j] = b++;
            i++;
        }
        i--; 
        // 填充底行（从右到左）
        j--;
        while(j >= 0) {
            t[i][j] = b++;
            j--;
        }
    } else {
        // 奇数情况：先填充底行，再填充左边列
        int i = a-1, j = 0;
        // 填充底行
        while(j < a) {
            t[i][j] = b++;
            j++;
        }
        j--; 
        // 填充左边列（从下到上）
        i--;
        while(i >= 0) {
            t[i][j] = b++;
            i--;
        }
    }
    
    return t;
}

void free_matrix(int** t, int n) {
    for(int i = 0; i < n; i++) {
        free(t[i]);
    }
    free(t);
}

int main(void) {
    int n;
    scanf("%d", &n);
    int** t = f(n);
    
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            printf("%d ", t[i][j]);
        }
        printf("\n");
    }
    
    free_matrix(t, n);  // 释放内存
    return 0;
}