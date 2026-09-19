#include <stdio.h>

// 每个数字需要的火柴棒数
int matchstick[10] = {6,2,5,5,4,5,6,3,7,6};

// 计算一个数需要多少根火柴棒
int count_match(int x) {
    if (x == 0) return matchstick[0];
    int cnt = 0;
    while (x > 0) {
        cnt += matchstick[x % 10];
        x /= 10;
    }
    return cnt;
}

int main() {
    int n, ans = 0;
    scanf("%d", &n);
    // 加号和等号共需4根火柴棒
    n -= 4;
    // 枚举A、B
    for (int A = 0; A < 1000; A++) {
        for (int B = 0; B < 1000; B++) {
            int C = A + B;
            int need = count_match(A) + count_match(B) + count_match(C);
            if (need == n) {
                ans++;
            }
        }
    }
    printf("%d\n", ans);
    return 0;
}