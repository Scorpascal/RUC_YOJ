#include <stdio.h>
#include <math.h>

static double sigmoid(double a) {
    return 1.0 / (1.0 + exp(-a));
}

int main(void) {
    int m, d, n;
    if (scanf("%d %d %d", &m, &d, &n) != 3) return 0;

    // 输入向量 x
    double x[32] = {0}; // 题目上限16，这里留余量
    for (int i = 0; i < m; ++i) {
        int xi;
        scanf("%d", &xi);
        x[i] = (double)xi;
    }

    // 参数矩阵 W (d x m)，按行输入
    double W[32][32] = {{0}};
    for (int i = 0; i < d; ++i) {
        for (int j = 0; j < m; ++j) {
            int wij;
            scanf("%d", &wij);
            W[i][j] = (double)wij;
        }
    }

    // 参数矩阵 V (n x d)，按行输入
    double V[32][32] = {{0}};
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < d; ++j) {
            int vij;
            scanf("%d", &vij);
            V[i][j] = (double)vij;
        }
    }

    // h = s(Wx)
    double h[32] = {0};
    for (int i = 0; i < d; ++i) {
        double sum = 0.0;
        for (int j = 0; j < m; ++j) {
            sum += W[i][j] * x[j];
        }
        h[i] = sigmoid(sum);
    }

    // o = s(Vh)
    double o[32] = {0};
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < d; ++j) {
            sum += V[i][j] * h[j];
        }
        o[i] = sigmoid(sum);
    }

    // 输出，保留两位小数，空格分隔
    for (int i = 0; i < n; ++i) {
        if (i) printf(" ");
        printf("%.2f", o[i]);
    }
    printf("\n");
    return 0;
}