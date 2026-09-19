#include <stdio.h>
#include <string.h>

static int score_hand_counts(const int cnt[7]) {
    // 优先级从高到低
    // 状元插金花：4个四 + 2个一
    if (cnt[4] == 4 && cnt[1] == 2) return 2048;

    // 红六勃：6个四
    if (cnt[4] == 6) return 1024;

    // 遍地锦：6个一
    if (cnt[1] == 6) return 512;

    // 黑六勃：6个相同（2、3、5、6）
    if (cnt[2] == 6 || cnt[3] == 6 || cnt[5] == 6 || cnt[6] == 6) return 256;

    // 五红：5个四
    if (cnt[4] == 5) return 128;

    // 五子：5个相同（非四），若附带1个四则可与一秀叠加
    for (int v = 1; v <= 6; ++v) {
        if (v == 4) continue;
        if (cnt[v] == 5) {
            if (cnt[4] == 1) return 64 + 1; // 五子64 + 一秀1
            return 64;
        }
    }

    // 状元：4个四，另两颗的点数相加
    if (cnt[4] == 4) {
        int sum = 0;
        for (int v = 1; v <= 6; ++v) {
            if (v == 4) continue;
            sum += v * cnt[v];
        }
        return 32 + sum;
    }

    // 对堂：1-6各一个
    int is_straight = 1;
    for (int v = 1; v <= 6; ++v) {
        if (cnt[v] != 1) { is_straight = 0; break; }
    }
    if (is_straight) return 16;

    // 四进（非四）：恰好4个相同（非四），可与二举或一秀叠加
    int four_in_non4 = 0;
    for (int v = 1; v <= 6; ++v) {
        if (v == 4) continue;
        if (cnt[v] == 4) { four_in_non4 = 1; break; }
    }
    if (four_in_non4) {
        int sc = 4;
        if (cnt[4] == 2) sc += 2;        // 二举叠加
        else if (cnt[4] == 1) sc += 1;   // 一秀叠加
        return sc;
    }

    // 三红：3个四
    if (cnt[4] == 3) return 8;

    // 二举：2个四
    if (cnt[4] == 2) return 2;

    // 一秀：1个四
    if (cnt[4] == 1) return 1;

    // 其他无分
    return 0;
}

static int score_hand(const int a[6]) {
    int cnt[7]; memset(cnt, 0, sizeof(cnt));
    for (int i = 0; i < 6; ++i) cnt[a[i]]++;
    return score_hand_counts(cnt);
}

static int score_from_multiset_indices(const int pool[12], const int idxs[6]) {
    int cnt[7]; memset(cnt, 0, sizeof(cnt));
    for (int i = 0; i < 6; ++i) cnt[ pool[idxs[i]] ]++;
    return score_hand_counts(cnt);
}

int main(void) {
    int jin[6], tu[6];
    for (int i = 0; i < 6; ++i) {
        if (scanf("%d", &jin[i]) != 1) return 0;
    }
    for (int i = 0; i < 6; ++i) {
        if (scanf("%d", &tu[i]) != 1) return 0;
    }

    // 组合成 12 个骰子的池
    int pool[12];
    for (int i = 0; i < 6; ++i) pool[i] = jin[i];
    for (int i = 0; i < 6; ++i) pool[6 + i] = tu[i];

    // 小金真实分
    int jin_true = score_hand(jin);

    // 枚举所有 C(12,6)=924 的划分
    // 用字典序下一个组合生成 idxs[0..5]
    int idxs[6] = {0,1,2,3,4,5};
    int best1 = 0;               // 宇宙1：只最大化小图
    int best2 = 0;               // 宇宙2：保证小金不减分，最大化小图

    int done = 0;
    while (!done) {
        int sc_tu = score_from_multiset_indices(pool, idxs);

        int cnt_all[7]; memset(cnt_all, 0, sizeof(cnt_all));
        for (int i = 0; i < 12; ++i) cnt_all[ pool[i] ]++;
        int cnt_tu[7]; memset(cnt_tu, 0, sizeof(cnt_tu));
        for (int i = 0; i < 6; ++i) cnt_tu[ pool[idxs[i]] ]++;
        int cnt_jin[7]; memset(cnt_jin, 0, sizeof(cnt_jin));
        for (int v = 1; v <= 6; ++v) cnt_jin[v] = cnt_all[v] - cnt_tu[v];
        int sc_jin_claim = score_hand_counts(cnt_jin);

        // 宇宙1更新
        if (sc_tu > best1) best1 = sc_tu;

        // 宇宙2约束：小金被分配到的组得分 >= 小金真实分
        if (sc_jin_claim >= jin_true && sc_tu > best2) best2 = sc_tu;

        // 生成下一个组合
        // 标准 next_combination
        int i;
        for (i = 5; i >= 0; --i) {
            if (idxs[i] != i + (12 - 6)) break;
        }
        if (i < 0) {
            done = 1;
        } else {
            idxs[i]++;
            for (int j = i + 1; j < 6; ++j) {
                idxs[j] = idxs[j - 1] + 1;
            }
        }
    }

    printf("%d %d\n", best1, best2);
    return 0;
}