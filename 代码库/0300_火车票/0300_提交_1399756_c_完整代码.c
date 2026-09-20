#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(void) {
    int N;
    if (scanf("%d", &N) != 1) return 0;

    // 占用计数：0->AB, 1->BC, 2->CD
    int occ[3] = {0, 0, 0};

    char buf[128];
    // 读掉行尾
    fgets(buf, sizeof(buf), stdin);

    while (fgets(buf, sizeof(buf), stdin)) {
        // 去掉行尾换行
        size_t len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) continue;

        // 输入结束
        if (len == 1 && buf[0] == '0') break;

        // 期望格式：<op><X><Y>，如 1AB、2BC 等
        if (len < 3) continue;

        int op = buf[0]; // '1' 买票, '2' 退票
        char s = buf[1];
        char t = buf[2];

        int delta = (op == '1') ? 1 : -1;

        // 规范化，确保 s < t，且范围在 A-D
        if (!(s >= 'A' && s <= 'D' && t >= 'A' && t <= 'D')) continue;
        if (s == t) continue;
        if (s > t) { char tmp = s; s = t; t = tmp; }

        // 根据区间覆盖增加/减少占用
        // A-B -> AB
        // B-C -> BC
        // C-D -> CD
        // A-C -> AB + BC
        // B-D -> BC + CD
        // A-D -> AB + BC + CD
        if (s == 'A' && t == 'B') { occ[0] += delta; }
        else if (s == 'B' && t == 'C') { occ[1] += delta; }
        else if (s == 'C' && t == 'D') { occ[2] += delta; }
        else if (s == 'A' && t == 'C') { occ[0] += delta; occ[1] += delta; }
        else if (s == 'B' && t == 'D') { occ[1] += delta; occ[2] += delta; }
        else if (s == 'A' && t == 'D') { occ[0] += delta; occ[1] += delta; occ[2] += delta; }
        // 其他不可能
    }

    // 可卖全程 AD 的票数 = 三段的最小空座
    int freeAB = N - occ[0];
    int freeBC = N - occ[1];
    int freeCD = N - occ[2];
    int ans = freeAB;
    if (freeBC < ans) ans = freeBC;
    if (freeCD < ans) ans = freeCD;

    if (ans < 0) ans = 0; // 保险

    printf("%d\n", ans);
    return 0;
}