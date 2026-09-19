#include <bits/stdc++.h>
using namespace std;

// 状态：到达当前点时，根到“父节点”为止路径上的最后一个有色点颜色
// NONE 表示路径上还没有任何有色点
static const int NONE = 0;
static const int BLACK = 1;
static const int WHITE = 2;
static const int INF = 1e9;

struct Adj {
    int to;
    int rev; // 在对方邻接表中的下标
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int m, n;
    if (!(cin >> m >> n)) return 0;

    // 约定：叶子为 1..n，颜色输入为 c[1..n]，0黑1白
    vector<int> reqState(m + 1, -1); // -1 表示无约束；否则为 BLACK/WHITE
    for (int i = 1; i <= n; i++) {
        int x;
        cin >> x;
        reqState[i] = (x == 0 ? BLACK : WHITE);
    }

    vector<vector<Adj>> adj(m + 1);
    adj.reserve(m + 1);

    for (int i = 0; i < m - 1; i++) {
        int a, b;
        cin >> a >> b;
        int ra = (int)adj[a].size();
        int rb = (int)adj[b].size();
        adj[a].push_back({b, rb});
        adj[b].push_back({a, ra});
    }

    vector<int> deg(m + 1, 0);
    for (int i = 1; i <= m; i++) deg[i] = (int)adj[i].size();

    // isConstrainedLeaf：题目“叶子节点”约束（按 1..n）且在树上确实是叶子（度=1）
    vector<char> isConstrainedLeaf(m + 1, 0);
    for (int i = 1; i <= n && i <= m; i++) {
        if (deg[i] == 1) isConstrainedLeaf[i] = 1;
    }

    // msg[v][eid][s]：从 v 通过邻接表下标 eid 指向的邻居“作为父方向”时，
    // 在切断该边后 v 所在连通块内满足约束的最小染色数，给定从父侧传入 v 的状态为 s。
    vector<vector<array<int, 3>>> msg(m + 1);
    for (int v = 1; v <= m; v++) msg[v].assign(adj[v].size(), {INF, INF, INF});

    // 任取一个根做两遍 DP（后序求子->父消息 + 前序 reroot 求父->子消息）
    int root0 = 1;

    vector<int> parent(m + 1, 0), parentEid(m + 1, -1);
    vector<int> order;
    order.reserve(m);

    // 建树（迭代 DFS）
    {
        vector<int> st;
        st.push_back(root0);
        parent[root0] = -1;
        while (!st.empty()) {
            int v = st.back();
            st.pop_back();
            order.push_back(v);
            for (int ei = 0; ei < (int)adj[v].size(); ei++) {
                int u = adj[v][ei].to;
                if (u == parent[v]) continue;
                parent[u] = v;
                parentEid[u] = adj[v][ei].rev; // 在 u 的邻接表里，指向 v 的那条边下标
                st.push_back(u);
            }
        }
    }

    auto solveMessage = [&](int v, int excludeNeighborIndex, const array<int,3>& sumExcl) -> array<int,3> {
        // excludeNeighborIndex：在 v 的邻接表里要排除的那一条边（发消息给它）
        // sumExcl[s]：对所有 x!=exclude 的邻居 x，msg[x->v][s] 之和
        array<int,3> res{INF, INF, INF};

        // 约束叶子：只能“自己染成指定色”或“不染但继承到的最后色==指定且不是 NONE”
        if (isConstrainedLeaf[v]) {
            int need = reqState[v]; // BLACK/WHITE
            for (int s = 0; s < 3; s++) {
                int noPaint = (s == need && s != NONE) ? 0 : INF;
                int paintSelf = 1; // 染成 need
                res[s] = min(noPaint, paintSelf);
            }
            return res;
        }

        // 普通点
        for (int s = 0; s < 3; s++) {
            int noPaint = sumExcl[s];
            int paintBlack = 1 + sumExcl[BLACK];
            int paintWhite = 1 + sumExcl[WHITE];
            res[s] = min({noPaint, paintBlack, paintWhite});
        }
        return res;
    };

    // 1) 后序：先算子->父消息（在 root0 的父子关系下）
    for (int idx = (int)order.size() - 1; idx >= 0; idx--) {
        int v = order[idx];
        if (v == root0) continue;

        int p = parent[v];
        int eidToP = parentEid[v]; // v 的邻接表中指向 p 的下标

        // sumExcl：排除 p 后，累加所有孩子 x->v 的消息
        array<int,3> sumExcl{0,0,0};
        for (int ei = 0; ei < (int)adj[v].size(); ei++) {
            if (ei == eidToP) continue;
            int x = adj[v][ei].to;
            int revInX = adj[v][ei].rev;
            for (int s = 0; s < 3; s++) {
                sumExcl[s] += msg[x][revInX][s];
            }
        }

        msg[v][eidToP] = solveMessage(v, eidToP, sumExcl);
    }

    // 2) 前序：用 reroot 算父->子消息（以及顺便也可重算子->父，保持一致）
    for (int v : order) {
        // totalSum[s] = sum over all neighbors x of msg[x->v][s]
        array<int,3> totalSum{0,0,0};
        for (int ei = 0; ei < (int)adj[v].size(); ei++) {
            int x = adj[v][ei].to;
            int revInX = adj[v][ei].rev;
            for (int s = 0; s < 3; s++) totalSum[s] += msg[x][revInX][s];
        }

        for (int ei = 0; ei < (int)adj[v].size(); ei++) {
            int u = adj[v][ei].to;
            int revInU = adj[v][ei].rev;

            // sumExcl = totalSum - msg[u->v]
            array<int,3> sumExcl = totalSum;
            for (int s = 0; s < 3; s++) sumExcl[s] -= msg[u][revInU][s];

            msg[v][ei] = solveMessage(v, ei, sumExcl);
        }
    }

    // 3) 枚举根（度>1），计算整棵树的最小值
    int answer = INF;
    for (int r = 1; r <= m; r++) {
        if (deg[r] <= 1) continue; // 根必须度数>1

        array<int,3> totalSum{0,0,0};
        for (int ei = 0; ei < (int)adj[r].size(); ei++) {
            int x = adj[r][ei].to;
            int revInX = adj[r][ei].rev;
            for (int s = 0; s < 3; s++) totalSum[s] += msg[x][revInX][s];
        }

        // 根的传入状态为 NONE
        int notPaint = totalSum[NONE];
        int paintBlack = 1 + totalSum[BLACK];
        int paintWhite = 1 + totalSum[WHITE];
        int best = min({notPaint, paintBlack, paintWhite});
        answer = min(answer, best);
    }

    if (answer >= INF/2) answer = 0; // 理论上不该发生（题目保证可选根）
    cout << answer << "\n";
    return 0;
}