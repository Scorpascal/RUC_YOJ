#include <stdio.h>

typedef long long ll;

int MOD = 10000;

// 矩阵结构体
typedef struct {
    int m[2][2];
} Matrix;

// 矩阵乘法
Matrix mul(Matrix a, Matrix b) {
    Matrix res;
    res.m[0][0] = (a.m[0][0]*b.m[0][0] + a.m[0][1]*b.m[1][0]) % MOD;
    res.m[0][1] = (a.m[0][0]*b.m[0][1] + a.m[0][1]*b.m[1][1]) % MOD;
    res.m[1][0] = (a.m[1][0]*b.m[0][0] + a.m[1][1]*b.m[1][0]) % MOD;
    res.m[1][1] = (a.m[1][0]*b.m[0][1] + a.m[1][1]*b.m[1][1]) % MOD;
    return res;
}

// 矩阵快速幂
Matrix powmat(Matrix base, ll n) {
    Matrix res = {{{1,0},{0,1}}}; // 单位矩阵
    while (n) {
        if (n & 1) res = mul(res, base);
        base = mul(base, base);
        n >>= 1;
    }
    return res;
}

int main() {
    ll n;
    scanf("%lld", &n);
    if (n == 0) {
        printf("0\n");
        return 0;
    }
    Matrix base = {{{1,1},{1,0}}};
    Matrix ans = powmat(base, n-1);
    int fn = ans.m[0][0]; // F[n] = ans[0][0]
    printf("%d\n", fn);
    return 0;
}