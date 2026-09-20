#include <iostream>
#include <vector>
#include <algorithm>
#include <stack>
#include <queue>

using namespace std;

static const long long INF = (1LL << 62);

struct DEdge {
    int to;
    vector<int> w; // size K*K, w[su*K + sv] = cost(u=su, v=sv)
};

struct UEdge {
    int a, b;
    vector<int> w; // oriented as read: a->b
};

static int K;

// transpose of KxK matrix (row-major)
static vector<int> transposeMat(const vector<int>& m) {
    vector<int> t(K * K);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < K; j++)
            t[j * K + i] = m[i * K + j];
    return t;
}

static const vector<int>* getWptr(const vector<vector<DEdge>>& g, int u, int v) {
    for (const auto& e : g[u]) if (e.to == v) return &e.w;
    return nullptr;
}

struct RootedTree {
    int n;
    int root;
    int banned;
    vector<int> parent;
    vector<vector<int>> children;
    vector<const vector<int>*> parentW; // parent -> node
    vector<int> order; // DFS order
};

static bool build_rooted_tree_ban(
    const vector<vector<DEdge>>& g,
    int root,
    int banned,
    RootedTree& out
) {
    int n = (int)g.size();
    out.n = n;
    out.root = root;
    out.banned = banned;
    out.parent.assign(n, -1);
    out.children.assign(n, {});
    out.parentW.assign(n, nullptr);
    out.order.clear();
    out.order.reserve(n);

    stack<int> st;
    out.parent[root] = root;
    st.push(root);
    while (!st.empty()) {
        int v = st.top();
        st.pop();
        out.order.push_back(v);
        for (const auto& e : g[v]) {
            int u = e.to;
            if (u == banned) continue;
            if (out.parent[u] != -1) continue;
            out.parent[u] = v;
            out.parentW[u] = &e.w;
            out.children[v].push_back(u);
            st.push(u);
        }
    }

    int need = n - (banned >= 0 ? 1 : 0);
    return (int)out.order.size() == need;
}

static vector<long long> transposeMatLL(const vector<long long>& m) {
    vector<long long> t(K * K);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < K; j++)
            t[j * K + i] = m[i * K + j];
    return t;
}

static vector<long long> minPlusMul(const vector<long long>& A, const vector<long long>& B) {
    // (KxK) min-plus multiply
    vector<long long> C(K * K, INF);
    for (int i = 0; i < K; i++) {
        for (int k = 0; k < K; k++) {
            long long best = INF;
            for (int j = 0; j < K; j++) {
                long long av = A[i * K + j];
                long long bv = B[j * K + k];
                if (av >= INF / 4 || bv >= INF / 4) continue;
                best = min(best, av + bv);
            }
            C[i * K + k] = best;
        }
    }
    return C;
}

// Tree DP on connected tree-like graph, possibly skipping one banned node.
// nodeCost[u][s] already includes station cost and any modifications.
static long long solve_tree_dp_ban(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& nodeCost,
    int root,
    int banned
) {
    RootedTree tr;
    if (!build_rooted_tree_ban(g, root, banned, tr)) return INF;

    vector<vector<long long>> dp(tr.n, vector<long long>(K, INF));
    for (int idx = (int)tr.order.size() - 1; idx >= 0; --idx) {
        int v = tr.order[idx];
        for (int sv = 0; sv < K; sv++) dp[v][sv] = nodeCost[v][sv];
        for (int u : tr.children[v]) {
            const auto& w = *tr.parentW[u];
            for (int sv = 0; sv < K; sv++) {
                long long best = INF;
                for (int su = 0; su < K; su++) best = min(best, dp[u][su] + (long long)w[sv * K + su]);
                dp[v][sv] += best;
            }
        }
    }

    long long ans = INF;
    for (int s = 0; s < K; s++) ans = min(ans, dp[root][s]);
    return ans;
}

static long long solve_bruteforce_small(
    int N,
    const vector<vector<long long>>& station,
    const vector<UEdge>& edges
) {
    // iterative base-K enumeration to avoid std::function recursion (faster compile)
    vector<int> st(N, 0);
    long long best = INF;
    while (true) {
        long long sum = 0;
        for (int v = 0; v < N; v++) sum += station[v][st[v]];
        for (const auto& e : edges) sum += e.w[st[e.a] * K + st[e.b]];
        best = min(best, sum);

        int p = 0;
        while (p < N) {
            if (++st[p] < K) break;
            st[p] = 0;
            p++;
        }
        if (p == N) break;
    }
    return best;
}

static long long solve_tree_case(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station
) {
    return solve_tree_dp_ban(g, station, 0, -1);
}

static long long solve_unicyclic_case(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station
) {
    int N = (int)g.size();
    vector<int> deg(N);
    for (int i = 0; i < N; i++) deg[i] = (int)g[i].size();

    vector<char> inCycle(N, 1);
    queue<int> q;
    for (int i = 0; i < N; i++) if (deg[i] == 1) q.push(i);

    while (!q.empty()) {
        int v = q.front(); q.pop();
        if (!inCycle[v]) continue;
        inCycle[v] = 0;
        for (const auto& e : g[v]) {
            int u = e.to;
            if (!inCycle[u]) continue;
            if (--deg[u] == 1) q.push(u);
        }
    }

    // build one cycle order by walking
    int start = -1;
    for (int i = 0; i < N; i++) if (inCycle[i]) { start = i; break; }
    if (start == -1) return INF;

    vector<int> cycle;
    vector<vector<int>> cycW; // cycW[t]: from cycle[t] -> cycle[(t+1)%L]
    int prev = -1, cur = start;
    while (true) {
        cycle.push_back(cur);
        int nxt = -1;
        vector<int> wcur;
        for (const auto& e : g[cur]) {
            if (inCycle[e.to] && e.to != prev) {
                nxt = e.to;
                wcur = e.w;
                break;
            }
        }
        if (nxt == -1) return INF; // malformed
        cycW.push_back(wcur);
        prev = cur;
        cur = nxt;
        if (cur == start) break;
        if ((int)cycle.size() > N + 5) return INF;
    }
    int L = (int)cycle.size();

    // forest DP from cycle nodes, excluding cycle edges
    vector<int> parent(N, -1);
    vector<vector<int>> children(N);
    vector<const vector<int>*> parentW(N, nullptr);
    vector<int> order;
    order.reserve(N);

    for (int c : cycle) parent[c] = c;

    stack<int> st;
    for (int c : cycle) {
        for (const auto& e : g[c]) {
            int u = e.to;
            if (inCycle[u]) continue;
            if (parent[u] != -1) continue;
            parent[u] = c;
            parentW[u] = &e.w; // c -> u
            children[c].push_back(u);
            st.push(u);
            while (!st.empty()) {
                int v = st.top(); st.pop();
                order.push_back(v);
                for (const auto& ee : g[v]) {
                    int x = ee.to;
                    if (inCycle[x]) continue;
                    if (parent[x] != -1) continue;
                    parent[x] = v;
                    parentW[x] = &ee.w; // v -> x
                    children[v].push_back(x);
                    st.push(x);
                }
            }
        }
    }

    vector<vector<long long>> dp(N, vector<long long>(K, INF));
    for (int idx = (int)order.size() - 1; idx >= 0; --idx) {
        int v = order[idx];
        for (int sv = 0; sv < K; sv++) dp[v][sv] = station[v][sv];
        for (int u : children[v]) {
            const auto& w = *parentW[u]; // v -> u
            for (int sv = 0; sv < K; sv++) {
                long long best = INF;
                for (int su = 0; su < K; su++) best = min(best, dp[u][su] + (long long)w[sv * K + su]);
                dp[v][sv] += best;
            }
        }
    }

    // base cost on cycle nodes (station + attached trees)
    vector<vector<long long>> base(N, vector<long long>(K, INF));
    for (int c : cycle) {
        for (int sc = 0; sc < K; sc++) base[c][sc] = station[c][sc];
        for (int u : children[c]) {
            const auto& w = *parentW[u]; // c -> u
            for (int sc = 0; sc < K; sc++) {
                long long best = INF;
                for (int su = 0; su < K; su++) best = min(best, dp[u][su] + (long long)w[sc * K + su]);
                base[c][sc] += best;
            }
        }
    }

    // cycle DP (enumerate start state)
    long long ans = INF;
    int c0 = cycle[0];
    for (int s0 = 0; s0 < K; s0++) {
        vector<long long> dpPrev(K, INF), dpCur(K, INF);
        dpPrev[s0] = base[c0][s0];

        for (int t = 1; t < L; t++) {
            fill(dpCur.begin(), dpCur.end(), INF);
            const auto& w = cycW[t - 1]; // cycle[t-1] -> cycle[t]
            int ct = cycle[t];
            for (int sj = 0; sj < K; sj++) {
                long long bestPrev = INF;
                for (int si = 0; si < K; si++) bestPrev = min(bestPrev, dpPrev[si] + (long long)w[si * K + sj]);
                dpCur[sj] = base[ct][sj] + bestPrev;
            }
            dpPrev.swap(dpCur);
        }
        // close last -> first
        const auto& wClose = cycW[L - 1]; // cycle[L-1] -> cycle[0]
        for (int sl = 0; sl < K; sl++) {
            ans = min(ans, dpPrev[sl] + (long long)wClose[sl * K + s0]);
        }
    }
    return ans;
}

static bool is_tree_after_removing(
    const vector<vector<DEdge>>& g,
    int removeV
) {
    int N = (int)g.size();
    int start = (removeV == 0 ? 1 : 0);
    if (start < 0 || start >= N) return false;

    vector<char> vis(N, 0);
    queue<int> q;
    vis[start] = 1;
    q.push(start);
    int cnt = 0;

    while (!q.empty()) {
        int v = q.front(); q.pop();
        cnt++;
        for (const auto& e : g[v]) {
            int u = e.to;
            if (u == removeV) continue;
            if (!vis[u]) { vis[u] = 1; q.push(u); }
        }
    }
    return cnt == N - 1;
}

static int find_feedback_vertex_one_by_bcc(int N, const vector<UEdge>& edges) {
    struct BccFinder {
        int n;
        const vector<UEdge>* edges;
        vector<vector<pair<int,int>>> adj;
        vector<int> disc, low, estack, mark;
        vector<char> inAll;
        int timer = 0;
        int tag = 0;
        bool hasCyclic = false;

        BccFinder(int N, const vector<UEdge>& E) : n(N), edges(&E) {
            int M = (int)E.size();
            adj.assign(n, {});
            for (int i = 0; i < M; i++) {
                int a = E[i].a, b = E[i].b;
                adj[a].push_back({b, i});
                adj[b].push_back({a, i});
            }
            disc.assign(n, 0);
            low.assign(n, 0);
            mark.assign(n, 0);
            inAll.assign(n, 1);
            estack.reserve(M);
        }

        void dfs(int u, int peid) {
            disc[u] = low[u] = ++timer;
            for (auto pr : adj[u]) {
                int v = pr.first;
                int eid = pr.second;
                if (!disc[v]) {
                    estack.push_back(eid);
                    dfs(v, eid);
                    low[u] = min(low[u], low[v]);
                    if (low[v] >= disc[u]) {
                        ++tag;
                        int ecount = 0;
                        int vcount = 0;
                        while (!estack.empty()) {
                            int x = estack.back();
                            estack.pop_back();
                            ecount++;
                            int a = (*edges)[x].a, b = (*edges)[x].b;
                            if (mark[a] != tag) { mark[a] = tag; vcount++; }
                            if (mark[b] != tag) { mark[b] = tag; vcount++; }
                            if (x == eid) break;
                        }
                        if (ecount >= vcount) {
                            if (!hasCyclic) {
                                fill(inAll.begin(), inAll.end(), 0);
                                for (int i = 0; i < n; i++) if (mark[i] == tag) inAll[i] = 1;
                                hasCyclic = true;
                            } else {
                                for (int i = 0; i < n; i++) {
                                    if (inAll[i] && mark[i] != tag) inAll[i] = 0;
                                }
                            }
                        }
                    }
                } else if (eid != peid && disc[v] < disc[u]) {
                    low[u] = min(low[u], disc[v]);
                    estack.push_back(eid);
                }
            }
        }

        int solve_any() {
            dfs(0, -1);
            if (!hasCyclic) return -1;
            for (int i = 0; i < n; i++) if (inAll[i]) return i;
            return -1;
        }
    };

    BccFinder bf(N, edges);
    return bf.solve_any();
}

static long long solve_feedback_vertex_one_case(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station,
    int D
) {
    int N = (int)g.size();
    int root = (D == 0 ? 1 : 0);

    RootedTree tr;
    if (!build_rooted_tree_ban(g, root, D, tr)) return INF;

    // precompute directed matrices from neighbor -> D
    vector<const vector<int>*> wToD(N, nullptr);
    for (int u = 0; u < N; u++) {
        if (u == D) continue;
        const vector<int>* wp = getWptr(g, u, D);
        if (wp) wToD[u] = wp;
    }

    long long best = INF;
    vector<vector<long long>> dp(N, vector<long long>(K, INF));

    for (int sd = 0; sd < K; sd++) {
        // bottom-up DP on fixed rooted tree
        for (int idx = (int)tr.order.size() - 1; idx >= 0; --idx) {
            int v = tr.order[idx];
            for (int sv = 0; sv < K; sv++) {
                long long base = station[v][sv];
                if (wToD[v]) base += (long long)(*wToD[v])[sv * K + sd];
                dp[v][sv] = base;
            }
            for (int u : tr.children[v]) {
                const auto& w = *tr.parentW[u];
                for (int sv = 0; sv < K; sv++) {
                    long long bestChild = INF;
                    for (int su = 0; su < K; su++) bestChild = min(bestChild, dp[u][su] + (long long)w[sv * K + su]);
                    dp[v][sv] += bestChild;
                }
            }
        }
        long long treePart = INF;
        for (int sr = 0; sr < K; sr++) treePart = min(treePart, dp[root][sr]);
        best = min(best, station[D][sd] + treePart);
    }

    return best;
}

static long long solve_degree_gt2_small_core_case(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station,
    const vector<UEdge>& undirectedEdges
) {
    int N = (int)g.size();
    vector<int> deg(N);
    for (int i = 0; i < N; i++) deg[i] = (int)g[i].size();

    vector<int> coreNodes;
    for (int i = 0; i < N; i++) if (deg[i] > 2) coreNodes.push_back(i);
    int B = (int)coreNodes.size();
    if (B == 0) return INF;
    if (B > 6) return INF;

    vector<int> coreId(N, -1);
    vector<char> isCore(N, 0);
    for (int i = 0; i < B; i++) {
        coreId[coreNodes[i]] = i;
        isCore[coreNodes[i]] = 1;
    }

    vector<vector<long long>> unary(B, vector<long long>(K, 0));
    for (int i = 0; i < B; i++) {
        int v = coreNodes[i];
        for (int s = 0; s < K; s++) unary[i][s] = station[v][s];
    }

    // pair cost in canonical order (i<j), matrix index: state_i * K + state_j
    vector<char> hasPair(B * B, 0);
    vector<vector<long long>> pairCost(B * B);
    auto pidx = [&](int i, int j) { return i * B + j; };
    for (int i = 0; i < B; i++) {
        for (int j = 0; j < B; j++) {
            if (i < j) pairCost[pidx(i, j)].assign(K * K, 0);
        }
    }

    auto addPairCanonical = [&](int na, int nb, const vector<long long>& mat_ab) {
        // mat_ab is oriented na -> nb, index: state_na*K + state_nb
        int ia = coreId[na], ib = coreId[nb];
        if (ia == -1 || ib == -1 || ia == ib) return;
        if (ia < ib) {
            hasPair[pidx(ia, ib)] = 1;
            auto& pc = pairCost[pidx(ia, ib)];
            for (int t = 0; t < K * K; t++) pc[t] += mat_ab[t];
        } else {
            // store as (ib, ia): need transpose
            auto mat_ba = transposeMatLL(mat_ab);
            hasPair[pidx(ib, ia)] = 1;
            auto& pc = pairCost[pidx(ib, ia)];
            for (int t = 0; t < K * K; t++) pc[t] += mat_ba[t];
        }
    };

    // 1) direct core-core edges
    for (const auto& e : undirectedEdges) {
        if (!isCore[e.a] || !isCore[e.b]) continue;
        vector<long long> mat(K * K);
        for (int t = 0; t < K * K; t++) mat[t] = (long long)e.w[t];
        addPairCanonical(e.a, e.b, mat);
    }

    // 2) components after removing core nodes are paths; fold them
    vector<char> vis(N, 0);

    auto buildStepMat = [&](int u, int v, bool includeStationOfV) -> vector<long long> {
        const vector<int>* wp = getWptr(g, u, v);
        vector<long long> mat(K * K, INF);
        for (int su = 0; su < K; su++) {
            for (int sv = 0; sv < K; sv++) {
                long long val = (long long)(*wp)[su * K + sv];
                if (includeStationOfV) val += station[v][sv];
                mat[su * K + sv] = val;
            }
        }
        return mat;
    };

    auto computePathMatrix = [&](const vector<int>& path, int endCore) -> vector<long long> {
        // path includes startCore as path[0] and ends at endCore as last element.
        // include station for internal nodes only (exclude both endpoints).
        vector<long long> cur(K * K, INF);
        for (int i = 0; i < K; i++) cur[i * K + i] = 0;
        for (int i = 0; i + 1 < (int)path.size(); i++) {
            int a = path[i], b = path[i + 1];
            bool includeB = (b != endCore);
            auto step = buildStepMat(a, b, includeB);
            cur = minPlusMul(cur, step);
        }
        return cur;
    };

    auto computeUnaryToLeaf = [&](const vector<int>& path) -> vector<long long> {
        // path includes startCore at [0], ends at a non-core leaf at last.
        vector<long long> cur(K * K, INF);
        for (int i = 0; i < K; i++) cur[i * K + i] = 0;
        for (int i = 0; i + 1 < (int)path.size(); i++) {
            int a = path[i], b = path[i + 1];
            auto step = buildStepMat(a, b, true); // include station on every non-core node, including leaf
            cur = minPlusMul(cur, step);
        }
        vector<long long> uni(K, INF);
        for (int sc = 0; sc < K; sc++) {
            long long best = INF;
            for (int sl = 0; sl < K; sl++) best = min(best, cur[sc * K + sl]);
            uni[sc] = best;
        }
        return uni;
    };

    for (int c : coreNodes) {
        for (const auto& e : g[c]) {
            int x = e.to;
            if (isCore[x]) continue;
            if (vis[x]) continue;

            vector<int> path;
            path.push_back(c);
            int prev = c;
            int cur = x;
            while (true) {
                path.push_back(cur);
                if (!isCore[cur]) vis[cur] = 1;

                if (isCore[cur]) break;
                if (deg[cur] == 1) break;

                // deg[cur] == 2, continue along the chain
                int nxt = -1;
                for (const auto& ee : g[cur]) {
                    if (ee.to != prev) { nxt = ee.to; break; }
                }
                if (nxt == -1) break;
                prev = cur;
                cur = nxt;
                if (!isCore[cur] && vis[cur]) {
                    // already processed by another core; stop
                    break;
                }
            }

            int endV = path.back();
            if (isCore[endV]) {
                auto mat = computePathMatrix(path, endV);
                if (endV == c) {
                    // cycle component that touches only this core node: add diagonal as unary
                    int ic = coreId[c];
                    for (int sc = 0; sc < K; sc++) unary[ic][sc] += mat[sc * K + sc];
                } else {
                    addPairCanonical(c, endV, mat);
                }
            } else {
                // ends at leaf
                auto uniAdd = computeUnaryToLeaf(path);
                int ic = coreId[c];
                for (int sc = 0; sc < K; sc++) unary[ic][sc] += uniAdd[sc];
            }
        }
    }

    // brute force on B<=6 core variables
    struct CoreEnum {
        int B;
        const vector<vector<long long>>* unary;
        const vector<char>* hasPair;
        const vector<vector<long long>>* pairCost;
        int K;
        vector<int> assign;
        long long best;
        CoreEnum(int b, int k,
                 const vector<vector<long long>>& u,
                 const vector<char>& hp,
                 const vector<vector<long long>>& pc)
            : B(b), unary(&u), hasPair(&hp), pairCost(&pc), K(k), assign(b, 0), best(INF) {}
        inline int pidx(int i, int j) const { return i * B + j; }
        void dfs(int i, long long curCost) {
            if (curCost >= best) return;
            if (i == B) {
                long long sum = curCost;
                for (int a = 0; a < B; a++) {
                    int sa = assign[a];
                    for (int b = a + 1; b < B; b++) {
                        int id = pidx(a, b);
                        if (!(*hasPair)[id]) continue;
                        sum += (*pairCost)[id][sa * K + assign[b]];
                        if (sum >= best) break;
                    }
                    if (sum >= best) break;
                }
                if (sum < best) best = sum;
                return;
            }
            for (int s = 0; s < K; s++) {
                assign[i] = s;
                dfs(i + 1, curCost + (*unary)[i][s]);
            }
        }
    };

    CoreEnum ce(B, K, unary, hasPair, pairCost);
    ce.dfs(0, 0);
    return ce.best;
}

static long long solve_overall(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station,
    const vector<UEdge>& undirectedEdges
) {
    int N = (int)g.size();
    int M = (int)undirectedEdges.size();

    // 1. Small N -> Brute Force
    if (N <= 6) return solve_bruteforce_small(N, station, undirectedEdges);

    // 2. Tree -> Tree DP
    if (M == N - 1) return solve_tree_case(g, station);

    // 3. Unicyclic -> Cycle DP + Tree DP
    if (M == N) return solve_unicyclic_case(g, station);

    // 4. Feedback vertex set size = 1 (2-core is a single node)
    // In this subtask, exists D such that removing D makes the remaining graph a tree.
    // Necessary condition: M - deg(D) == (N-1) - 1 => deg(D) == M - N + 2.
    // Try candidates by degree first (usually very few), then fall back to BCC intersection.
    {
        vector<int> deg(N);
        for (int i = 0; i < N; i++) deg[i] = (int)g[i].size();
        int needDeg = M - N + 2;
        if (needDeg >= 0) {
            for (int D = 0; D < N; D++) {
                if (deg[D] != needDeg) continue;
                if (M - deg[D] != N - 2) continue;
                if (!is_tree_after_removing(g, D)) continue;
                long long ans = solve_feedback_vertex_one_case(g, station, D);
                if (ans < INF) return ans;
            }
        }

        int D = find_feedback_vertex_one_by_bcc(N, undirectedEdges);
        if (D != -1) {
            if (M - deg[D] == N - 2 && is_tree_after_removing(g, D)) {
                long long ans = solve_feedback_vertex_one_case(g, station, D);
                if (ans < INF) return ans;
            }
        }
    }

    // 5. Degree > 2 nodes <= 6
    long long ans2 = solve_degree_gt2_small_core_case(g, station, undirectedEdges);
    if (ans2 < INF) return ans2;

    return INF;
}

long long solve(
    const vector<vector<DEdge>>& g,
    const vector<vector<long long>>& station,
    const vector<UEdge>& undirectedEdges
) {
    // reset global K
    K = (int)station[0].size();

    return solve_overall(g, station, undirectedEdges);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    if (!(cin >> N >> M >> K)) return 0;

    vector<vector<long long>> station(N, vector<long long>(K));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < K; j++) {
            cin >> station[i][j];
        }
    }

    vector<vector<DEdge>> g(N);
    vector<UEdge> undirectedEdges;
    undirectedEdges.reserve(M);

    for (int i = 0; i < M; i++) {
        int u, v;
        cin >> u >> v;
        vector<int> w(K * K);
        for (int j = 0; j < K * K; j++) {
            cin >> w[j];
        }
        
        g[u].push_back({v, w});
        g[v].push_back({u, transposeMat(w)});
        
        undirectedEdges.push_back({u, v, w});
    }

    cout << solve(g, station, undirectedEdges) << endl;

    return 0;
}

