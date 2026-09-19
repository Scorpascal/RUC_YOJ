#include <stdio.h>

static inline int min_int(int a, int b){ return a < b ? a : b; }
static inline int max_int(int a, int b){ return a > b ? a : b; }

int main(void){
    int N;
    if(scanf("%d", &N) != 1) return 0;

    // N <= 10
    int A[10], B[10], C[10];
    int gDigits[10][4];   // 每条猜测的4位数字
    int gCnt[10][10];     // 每条猜测的数字频次

    // 读取并预处理
    for(int i=0;i<N;i++){
        scanf("%d%d%d", &A[i], &B[i], &C[i]);
        int x = A[i];
        // 拆分为 4 位（保证为 1000..9999）
        gDigits[i][3] = x % 10; x /= 10;
        gDigits[i][2] = x % 10; x /= 10;
        gDigits[i][1] = x % 10; x /= 10;
        gDigits[i][0] = x % 10;

        for(int d=0; d<10; d++) gCnt[i][d] = 0;
        for(int k=0; k<4; k++) gCnt[i][ gDigits[i][k] ]++;
    }

    // 若存在 C=4：验证一致性，唯一则直接返回；若冲突则直接 Not sure
    int idxC4 = -1, cntC4 = 0;
    for(int i=0;i<N;i++){
        if(C[i] == 4){ idxC4 = i; cntC4++; }
    }
    if(cntC4 > 0){
        int okCount = 0;
        int candidateAns = -1;
        for(int t=0;t<N;t++){
            if(C[t] != 4) continue;
            int v = A[t];
            int d[4] = { v/1000, (v/100)%10, (v/10)%10, v%10 };
            int cCnt[10] = {0};
            for(int k=0;k<4;k++) cCnt[d[k]]++;

            int consistent = 1;
            for(int i=0;i<N && consistent;i++){
                int inPlace = (d[0]==gDigits[i][0]) + (d[1]==gDigits[i][1]) +
                              (d[2]==gDigits[i][2]) + (d[3]==gDigits[i][3]);
                int common = 0;
                for(int dig=0; dig<10; dig++){
                    common += min_int(cCnt[dig], gCnt[i][dig]);
                }
                if(inPlace != C[i] || common != B[i]) consistent = 0;
            }
            if(consistent){
                okCount++;
                candidateAns = v;
            }
        }
        if(okCount == 1){
            printf("%d\n", candidateAns);
            return 0;
        }else{
            printf("Not sure\n");
            return 0;
        }
    }

    // 基于 B=0 的猜测做数字排除剪枝（这些猜测中的任何数字都不可能出现）
    unsigned short bannedMask = 0; // 10 位 bitmask
    for(int i=0;i<N;i++){
        if(B[i] == 0){ // 若 B=0，则该猜测的4个数字都不在目标中
            for(int k=0;k<4;k++){
                bannedMask |= (1u << gDigits[i][k]);
            }
        }
    }

    // 基于所有行 B 的频次上下界剪枝
    int LB[10], UB[10];
    for(int d=0; d<10; d++){ LB[d]=0; UB[d]=4; }

    // 若出现 B=4：目标数的多重集与该行完全相同
    int hasB4 = 0, refIdx = -1;
    for(int i=0;i<N;i++){
        if(B[i] == 4){
            if(!hasB4){
                hasB4 = 1; refIdx = i;
                for(int d=0; d<10; d++){
                    LB[d] = max_int(LB[d], gCnt[i][d]);
                    UB[d] = gCnt[i][d] < UB[d] ? gCnt[i][d] : UB[d];
                }
            }else{
                for(int d=0; d<10; d++){
                    if(gCnt[i][d] != gCnt[refIdx][d]){
                        printf("Not sure\n");
                        return 0;
                    }
                }
            }
        }
    }

    // 来自所有行的通用上下界（安全、不失真）
    for(int i=0;i<N;i++){
        for(int d=0; d<10; d++){
            // 下界：min(s_d, gCnt[i][d]) >= max(0, B[i] - (4 - gCnt[i][d])) ⇒ s_d >= 该值
            int L = B[i] - (4 - gCnt[i][d]);
            if(L < 0) L = 0;
            if(L > LB[d]) LB[d] = L;

            // 上界：若 gCnt[i][d] > B[i]，否则 s_d 不可能 ≥ gCnt[i][d]（否则 min > B[i]）
            if(gCnt[i][d] > B[i]){
                if(UB[d] > B[i]) UB[d] = B[i];
            }
        }
    }

    // 应用 bannedMask：这些数字绝不出现
    for(int d=0; d<10; d++){
        if(bannedMask & (1u<<d)){
            LB[d] = 0;
            UB[d] = 0;
        }
    }

    // 早期可行性检查：∑LB ≤ 4 ≤ ∑UB
    int sumLB = 0, sumUB = 0;
    for(int d=0; d<10; d++){ sumLB += LB[d]; sumUB += UB[d]; }
    if(sumLB > 4 || sumUB < 4){
        // 约束矛盾，无解
        printf("Not sure\n");
        return 0;
    }

    int answers[9000];
    int q = 0;

    for(int v = 1000; v <= 9999; v++){
        // 拆分候选数字
        int d[4];
        int t = v;
        d[3] = t % 10; t /= 10;
        d[2] = t % 10; t /= 10;
        d[1] = t % 10; t /= 10;
        d[0] = t % 10;

        // banned 剪枝
        if( (bannedMask & (1u << d[0])) ||
            (bannedMask & (1u << d[1])) ||
            (bannedMask & (1u << d[2])) ||
            (bannedMask & (1u << d[3])) ){
            continue;
        }

        // 统计候选频次
        int cCnt[10] = {0};
        cCnt[d[0]]++; cCnt[d[1]]++; cCnt[d[2]]++; cCnt[d[3]]++;

        // 频次上下界剪枝
        int passFreqBounds = 1;
        for(int dig=0; dig<10; dig++){
            if(cCnt[dig] < LB[dig] || cCnt[dig] > UB[dig]){ passFreqBounds = 0; break; }
        }
        if(!passFreqBounds) continue;

        int ok = 1;
        for(int i=0;i<N && ok;i++){
            // 位置正确数
            int inPlace = (d[0]==gDigits[i][0]) + (d[1]==gDigits[i][1]) +
                          (d[2]==gDigits[i][2]) + (d[3]==gDigits[i][3]);
            if(inPlace != C[i]) { ok = 0; break; }

            // 公共数字总数
            int common = 0;
            for(int dig=0; dig<10; dig++){
                common += min_int(cCnt[dig], gCnt[i][dig]);
            }
            if(common != B[i]) { ok = 0; break; }
        }

        if(ok) answers[q++] = v;
    }

    if(q == 1) {
        printf("%d\n", answers[0]);
    } else {
        printf("Not sure\n");
    }
    return 0;
}