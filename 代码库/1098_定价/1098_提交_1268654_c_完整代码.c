#include <stdio.h>

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    unsigned long long pow10[11];
    pow10[0] = 1;
    for (int i = 1; i <= 10; ++i) pow10[i] = pow10[i-1] * 10ULL;

    while (T--) {
        unsigned long long L, R;
        scanf("%llu %llu", &L, &R);

        unsigned long long answer = 0;
        for (int a = 1; a <= 10 && answer == 0; ++a) {
            unsigned long long lowX = pow10[a-1];
            unsigned long long highX = pow10[a] - 1;
            unsigned long long best5 = 0;
            unsigned long long bestNon5 = 0;

            for (int k = 0; lowX * pow10[k] <= R; ++k) {
                unsigned long long mult = pow10[k];
                unsigned long long segLow = lowX * mult;
                unsigned long long segHigh = highX * mult;
                if (segHigh < L) continue;
                if (segLow > R) break;

                unsigned long long d_low = (L + mult - 1) / mult;
                unsigned long long d_high = R / mult;
                if (d_low < lowX) d_low = lowX;
                if (d_high > highX) d_high = highX;
                if (d_low > d_high) continue;

                // 找以 5 结尾的最小 d
                unsigned long long rem = d_low % 10ULL;
                unsigned long long delta5 = ( (5 + 10ULL - rem) % 10ULL );
                unsigned long long d5 = d_low + delta5;
                if (d5 <= d_high && (best5 == 0 || d5 * mult < best5))
                    best5 = d5 * mult;

                // 找最小的末位非 0 非 5 的 d
                unsigned long long d = d_low;
                // 调整到末位非 0
                if (d % 10ULL == 0) {
                    if (d + 1 <= d_high) d += 1;
                    else continue;
                }
                // 若末位是 5，尝试下一个符合条件的
                if (d % 10ULL == 5) {
                    unsigned long long d2 = d + 1; // 6
                    if (d2 <= d_high && d2 % 10ULL != 0 && d2 % 10ULL != 5) d = d2;
                    else {
                        // 跳到下一个可能：循环前进直到找到末位非 0 非 5
                        unsigned long long cur = d;
                        int found = 0;
                        for (int step = 1; step < 10 && cur + step <= d_high; ++step) {
                            unsigned long long cand = cur + step;
                            unsigned long long last = cand % 10ULL;
                            if (last != 0 && last != 5) { d = cand; found = 1; break; }
                        }
                        if (!found) continue;
                    }
                }
                unsigned long long last = d % 10ULL;
                if (last != 0 && last != 5) {
                    unsigned long long pNon5 = d * mult;
                    if (bestNon5 == 0 || pNon5 < bestNon5)
                        bestNon5 = pNon5;
                }
            }

            if (best5) {
                answer = best5;
                break;
            } else if (bestNon5) {
                answer = bestNon5;
                break;
            }
        }
        // 理论上必定找到
        printf("%llu\n", answer);
    }
    return 0;
}