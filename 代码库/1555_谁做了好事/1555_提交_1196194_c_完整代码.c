#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    char names[4] = {'A','B','C','D'};
    for (int cand = 0; cand < 4; ++cand) {
        int a_true = (cand != 0);   // A: "不是我"
        int b_true = (cand == 2);   // B: "是C"
        int c_true = (cand == 3);   // C: "是D"
        int d_true = !c_true;       // D: "C胡说" -> C为假时为真
        int cnt = a_true + b_true + c_true + d_true;
        if (cnt == n) printf("%c\n", names[cand]);
    }
    return 0;
}