#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

struct Frame {
	int u;
	int idx;
};

static inline void fast_io() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
}

int main() {
	fast_io();

	int n, m, q;
	cin >> n >> m >> q;

	vector<int> eu(m + 1), ev(m + 1);
	vector<vector<pair<int, int>>> adj(n + 1);
	adj.reserve(n + 1);
	for (int i = 1; i <= m; i++) {
		int u, v;
		cin >> u >> v;
		eu[i] = u;
		ev[i] = v;
		adj[u].push_back({v, i});
		adj[v].push_back({u, i});
	}

	// Iterative Tarjan for vertex-biconnected components (block-cut tree)
	vector<int> disc(n + 1, 0), low(n + 1, 0);
	vector<int> parent(n + 1, 0), parentEdge(n + 1, 0), rootOf(n + 1, 0);
	vector<int> childCnt(n + 1, 0);
	vector<char> isArt(n + 1, 0);

	vector<vector<int>> compsOf(n + 1);
	vector<int> lastSeen(n + 1, 0);

	vector<int> edgeStack;
	edgeStack.reserve(m);

	int dfsTime = 0;
	int bccCnt = 0;
	vector<int> roots;
	roots.reserve(n);

	auto make_bcc = [&](int stopEdgeId) {
		++bccCnt;
		vector<int> verts;
		verts.reserve(8);
		while (!edgeStack.empty()) {
			int eid = edgeStack.back();
			edgeStack.pop_back();
			int a = eu[eid], b = ev[eid];
			if (lastSeen[a] != bccCnt) {
				lastSeen[a] = bccCnt;
				verts.push_back(a);
			}
			if (lastSeen[b] != bccCnt) {
				lastSeen[b] = bccCnt;
				verts.push_back(b);
			}
			if (eid == stopEdgeId) break;
		}
		for (int v : verts) compsOf[v].push_back(bccCnt);
	};

	vector<int> iterPos(n + 1, 0);
	vector<Frame> st;
	st.reserve(n);

	auto discover = [&](int v, int p, int pe, int root) {
		disc[v] = low[v] = ++dfsTime;
		parent[v] = p;
		parentEdge[v] = pe;
		rootOf[v] = root;
		iterPos[v] = 0;
		st.push_back({v, 0});
	};

	for (int start = 1; start <= n; start++) {
		if (disc[start] != 0) continue;
		roots.push_back(start);
		discover(start, 0, 0, start);

		while (!st.empty()) {
			int u = st.back().u;
			int &idx = st.back().idx;

			if (idx >= (int)adj[u].size()) {
				// finish u
				int p = parent[u];
				int peid = parentEdge[u];
				st.pop_back();
				if (p != 0) {
					// update parent
					low[p] = min(low[p], low[u]);
					if (low[u] >= disc[p]) {
						if (p != rootOf[p]) isArt[p] = 1;
						make_bcc(peid);
					}
				}
				continue;
			}

			auto [v, eid] = adj[u][idx++];
			if (eid == parentEdge[u]) continue;

			if (disc[v] == 0) {
				childCnt[u]++;
				edgeStack.push_back(eid);
				discover(v, u, eid, rootOf[u]);
			} else if (disc[v] < disc[u]) {
				edgeStack.push_back(eid);
				low[u] = min(low[u], disc[v]);
			}
		}
	}

	// finalize root articulation status
	for (int r : roots) {
		if (childCnt[r] >= 2) isArt[r] = 1;
	}

	// Build block-cut tree (BC-tree)
	int artCnt = 0;
	vector<int> artNodeId(n + 1, 0);
	for (int v = 1; v <= n; v++) {
		if (isArt[v]) {
			artNodeId[v] = ++artCnt;
		}
	}

	int bcN = bccCnt + artCnt;
	vector<vector<int>> bcTree(bcN + 1);
	vector<char> isArtNode(bcN + 1, 0);

	// remap articulation node ids to [bccCnt+1 .. bccCnt+artCnt]
	for (int v = 1; v <= n; v++) {
		if (!isArt[v]) continue;
		artNodeId[v] = bccCnt + artNodeId[v];
		isArtNode[artNodeId[v]] = 1;
	}

	vector<int> belong(n + 1, 0);
	for (int v = 1; v <= n; v++) {
		if (isArt[v]) {
			int aId = artNodeId[v];
			for (int comp : compsOf[v]) {
				bcTree[aId].push_back(comp);
				bcTree[comp].push_back(aId);
			}
		} else {
			// non-articulation belongs to exactly one biconnected component
			belong[v] = compsOf[v].empty() ? 0 : compsOf[v][0];
		}
	}

	// LCA preprocessing on BC-tree
	const int LOG = 20;
	vector<array<int, LOG>> up(bcN + 1);
	vector<int> depth(bcN + 1, 0);
	vector<int> tin(bcN + 1, 0), tout(bcN + 1, 0);
	vector<int> artPref(bcN + 1, 0);
	vector<int> it2(bcN + 1, 0);

	int timer = 0;
	int root = 1;
	// BC-tree is connected when original graph is connected; still be safe
	for (int s = 1; s <= bcN; s++) {
		if (tin[s] != 0) continue;
		root = s;
		vector<int> stack;
		stack.push_back(root);
		up[root].fill(0);
		depth[root] = 0;
		artPref[root] = isArtNode[root] ? 1 : 0;
		tin[root] = ++timer;
		while (!stack.empty()) {
			int u = stack.back();
			if (it2[u] >= (int)bcTree[u].size()) {
				tout[u] = ++timer;
				stack.pop_back();
				continue;
			}
			int v = bcTree[u][it2[u]++];
			if (v == up[u][0]) continue;
			up[v].fill(0);
			up[v][0] = u;
			depth[v] = depth[u] + 1;
			artPref[v] = artPref[u] + (isArtNode[v] ? 1 : 0);
			tin[v] = ++timer;
			stack.push_back(v);
		}
	}

	for (int j = 1; j < LOG; j++) {
		for (int v = 1; v <= bcN; v++) {
			int mid = up[v][j - 1];
			up[v][j] = mid ? up[mid][j - 1] : 0;
		}
	}

	auto is_ancestor = [&](int a, int b) -> bool {
		return tin[a] <= tin[b] && tout[b] <= tout[a];
	};

	auto lca = [&](int a, int b) -> int {
		if (a == 0 || b == 0) return a ^ b;
		if (is_ancestor(a, b)) return a;
		if (is_ancestor(b, a)) return b;
		for (int j = LOG - 1; j >= 0; j--) {
			int pa = up[a][j];
			if (pa && !is_ancestor(pa, b)) a = pa;
		}
		return up[a][0];
	};

	auto count_art_on_path = [&](int a, int b) -> int {
		int L = lca(a, b);
		int cnt = artPref[a] + artPref[b] - 2 * artPref[L] + (isArtNode[L] ? 1 : 0);
		return cnt;
	};

	auto cmpTin = [&](int a, int b) {
		return tin[a] < tin[b];
	};

	// Answer queries
	for (int qi = 0; qi < q; qi++) {
		int k;
		cin >> k;
		vector<int> mapped;
		mapped.reserve(k);

		unordered_map<int, int> wMap;
		wMap.reserve((size_t)k * 2 + 8);

		for (int i = 0; i < k; i++) {
			int v;
			cin >> v;
			int node;
			if (isArt[v]) node = artNodeId[v];
			else node = belong[v];
			++wMap[node];
			mapped.push_back(node);
		}

		// Unique museum nodes in BC-tree
		sort(mapped.begin(), mapped.end(), cmpTin);
		mapped.erase(unique(mapped.begin(), mapped.end()), mapped.end());

		// If 0 or 1 museum node, all articulation deltas are 0
		if (k == 0) {
			cout << 0 << "\n";
			continue;
		}

		vector<int> nodes = mapped;
		// add LCAs
		for (int i = 0; i + 1 < (int)mapped.size(); i++) {
			nodes.push_back(lca(mapped[i], mapped[i + 1]));
		}
		sort(nodes.begin(), nodes.end(), cmpTin);
		nodes.erase(unique(nodes.begin(), nodes.end()), nodes.end());

		// Build virtual tree edges (parent -> child)
		vector<pair<int, int>> edges;
		edges.reserve(nodes.size());
		vector<int> stV;
		stV.reserve(nodes.size());
		stV.push_back(nodes[0]);

		for (int i = 1; i < (int)nodes.size(); i++) {
			int u = nodes[i];
			int L = lca(u, stV.back());
			if (L == stV.back()) {
				stV.push_back(u);
				continue;
			}
			while (stV.size() >= 2 && depth[stV[stV.size() - 2]] >= depth[L]) {
				edges.push_back({stV[stV.size() - 2], stV.back()});
				stV.pop_back();
			}
			if (stV.back() != L) {
				edges.push_back({L, stV.back()});
				stV.pop_back();
				if (stV.empty() || stV.back() != L) stV.push_back(L);
			}
			stV.push_back(u);
		}
		while (stV.size() >= 2) {
			edges.push_back({stV[stV.size() - 2], stV.back()});
			stV.pop_back();
		}
		int vtRoot = stV.front();

		// Index nodes for per-query arrays
		unordered_map<int, int> idx;
		idx.reserve(nodes.size() * 2 + 8);
		for (int i = 0; i < (int)nodes.size(); i++) idx[nodes[i]] = i;

		vector<vector<int>> children(nodes.size());
		for (auto [p, c] : edges) {
			children[idx[p]].push_back(idx[c]);
		}

		vector<int> w(nodes.size(), 0);
		for (int i = 0; i < (int)nodes.size(); i++) {
			auto it = wMap.find(nodes[i]);
			if (it != wMap.end()) w[i] = it->second;
		}

		// Compute subtree sums on virtual tree (iterative postorder)
		vector<int> order;
		order.reserve(nodes.size());
		vector<int> stk;
		stk.reserve(nodes.size());
		stk.push_back(idx[vtRoot]);
		while (!stk.empty()) {
			int u = stk.back();
			stk.pop_back();
			order.push_back(u);
			for (int c : children[u]) stk.push_back(c);
		}
		vector<int> sub(nodes.size(), 0);
		for (int t = (int)order.size() - 1; t >= 0; t--) {
			int u = order[t];
			int sum = w[u];
			for (int c : children[u]) sum += sub[c];
			sub[u] = sum;
		}

		long long deltaSum = 0;
		// 1) Contributions from articulation nodes explicitly present in virtual tree
		for (int i = 0; i < (int)nodes.size(); i++) {
			int nodeId = nodes[i];
			if (!isArtNode[nodeId]) continue;

			int maxChild = 0;
			for (int c : children[i]) maxChild = max(maxChild, sub[c]);
			int parentSide = k - sub[i];
			int maxComp = max(maxChild, parentSide);
			int totalExcl = k - w[i];
			deltaSum += (long long)maxComp - (long long)totalExcl;
		}

		// 2) Contributions from articulation nodes suppressed on compressed virtual edges
		// For each virtual edge parent->child, every internal articulation node on the
		// original BC-tree path has the same split: sub[child] vs (k - sub[child]).
		for (auto [pId, cId] : edges) {
			int p = idx[pId];
			int c = idx[cId];
			int wSide = sub[c];
			int other = k - wSide;
			int cnt = count_art_on_path(pId, cId);
			if (isArtNode[pId]) cnt--;
			if (isArtNode[cId]) cnt--;
			if (cnt > 0) {
				deltaSum -= (long long)cnt * (long long)min(wSide, other);
			}
		}

		long long ans = (long long)(n - 1) * (long long)k + deltaSum;
		cout << ans << "\n";
	}

	return 0;
}