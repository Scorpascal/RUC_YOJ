// 题意：杯子为等腰圆台，按顺序套入直到最深可行位置，求使用全部杯子能得到的最大塔高。
// 做法：
// 1) 预先计算 shift[i][j]：将杯 j 套入杯 i 需要的最小下沉距离 s（若不可行则为 INF）。
//    半径随高线性：r(x)=r0 + k*x，差值线性，只需检查重叠区间端点。
//    分两段：A) j 完全在 i 内 (s ≤ h_i - h_j)；B) j 高于 i 顶部 (s > h_i - h_j)。
//    在每段对不等式 k*s ≥ d 形式做区间收缩得到最小可行 s。
// 2) n≤10，枚举全排列深度优先：依次把新杯套入当前最顶杯，累加位移 offset，更新全局最大高度。
//    状态：当前顶杯 last，顶杯底部相对塔底的位置 offset，当前塔高 curH。
// 3) 输出最大高度，误差 1e-4 即可。
#include <bits/stdc++.h>
using namespace std;

const double INF = 1e100;
const double EPS = 1e-9;

int n;
double r0[10], R0[10], h0[10], k[10], topR[10];
double shiftDist[10][10];
double ans = 0;

bool tighten(double c, double rhs, double &L, double &U) {
    if (fabs(c) < EPS) return rhs <= EPS;
    if (c > 0) L = max(L, rhs / c);
    else       U = min(U, rhs / c);
    return L <= U + EPS;
}

// 最小可行下沉距离，将 inner 套入 outer
double minShift(int outer, int inner) {
    double rb = r0[outer], RB = topR[outer], hb = h0[outer], kb = k[outer];
    double ra = r0[inner], Ra = topR[inner], ha = h0[inner], ka = k[inner];

    // 底半径必须小于外杯口半径，否则永不可行
    if (ra - RB > EPS) return INF;

    double best = INF;

    // 段 A：ha 全部在 hb 内，s ∈ [0, hb - ha]
    if (hb - ha >= -EPS) {
        double L = 0, U = hb - ha; if (U < 0) U = 0;
        bool ok = tighten(kb, ra - rb, L, U); // ra <= r(b,s)
        ok &= tighten(kb, (Ra) - rb - kb * ha, L, U); // Ra <= r(b,s+ha)
        if (ok && L <= U + EPS) best = min(best, L);
    }

    // 段 B：inner 高出 outer 顶部，s ∈ [max(0,hb-ha), hb]
    {
        double L = max(0.0, hb - ha), U = hb;
        bool ok = tighten(kb, ra - rb, L, U); // ra <= r(b,s)
        ok &= tighten(ka, ka * hb + ra - RB, L, U); // rA(hb-s) <= RB
        if (ok && L <= U + EPS) best = min(best, L);
    }

    return best;
}

void dfs(int depth, int last, double offset, double curH, int mask) {
    if (depth == n) {
        ans = max(ans, curH);
        return;
    }
    for (int i = 0; i < n; ++i) if (!(mask & (1 << i))) {
        double s = shiftDist[last][i];
        if (s > INF / 2) continue; // 不可套入
        double newOffset = offset + s;
        double newH = max(curH, newOffset + h0[i]);
        dfs(depth + 1, i, newOffset, newH, mask | (1 << i));
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (!(cin >> n)) return 0;
    for (int i = 0; i < n; ++i) {
        cin >> r0[i] >> R0[i] >> h0[i];
        if (h0[i] == 0) k[i] = 0;
        else k[i] = (R0[i] - r0[i]) / h0[i];
        topR[i] = R0[i];
    }

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            shiftDist[i][j] = (i == j) ? INF : minShift(i, j);

    ans = 0;
    for (int i = 0; i < n; ++i) {
        dfs(1, i, 0.0, h0[i], 1 << i);
    }

    cout.setf(ios::fixed);
    cout << setprecision(6) << ans << "\n";
    return 0;
}