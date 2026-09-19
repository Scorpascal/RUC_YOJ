
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <queue>
#include <vector>
using namespace std;

struct Dinic {
	struct Edge {
		int to;
		int rev;
		long long cap;
	};

	int n;
	vector<vector<Edge>> g;
	vector<int> level;
	vector<int> it;

	explicit Dinic(int n_) : n(n_), g(n_), level(n_), it(n_) {}

	int add_edge(int fr, int to, long long cap) {
		Edge a{to, (int)g[to].size(), cap};
		Edge b{fr, (int)g[fr].size(), 0};
		g[fr].push_back(a);
		g[to].push_back(b);
		return (int)g[fr].size() - 1;
	}

	bool bfs(int s, int t) {
		fill(level.begin(), level.end(), -1);
		queue<int> q;
		level[s] = 0;
		q.push(s);
		while (!q.empty()) {
			int v = q.front();
			q.pop();
			for (const auto &e : g[v]) {
				if (e.cap <= 0) continue;
				if (level[e.to] != -1) continue;
				level[e.to] = level[v] + 1;
				q.push(e.to);
			}
		}
		return level[t] != -1;
	}

	long long dfs(int v, int t, long long f) {
		if (v == t) return f;
		for (int &i = it[v]; i < (int)g[v].size(); i++) {
			Edge &e = g[v][i];
			if (e.cap <= 0) continue;
			if (level[e.to] != level[v] + 1) continue;
			long long pushed = dfs(e.to, t, min(f, e.cap));
			if (pushed <= 0) continue;
			e.cap -= pushed;
			g[e.to][e.rev].cap += pushed;
			return pushed;
		}
		return 0;
	}

	long long maxflow(int s, int t) {
		long long flow = 0;
		while (bfs(s, t)) {
			fill(it.begin(), it.end(), 0);
			while (true) {
				long long pushed = dfs(s, t, (1LL << 62));
				if (pushed == 0) break;
				flow += pushed;
			}
		}
		return flow;
	}
};

static void disable_node(Dinic &dinic, int v) {
	for (auto &e : dinic.g[v]) {
		e.cap = 0;
		Dinic::Edge &rev = dinic.g[e.to][e.rev];
		rev.cap = 0;
	}
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n, m, s, t;
	if (!(cin >> n >> m >> s >> t)) return 0;

	const long long INF = (1LL << 60);
	int SS = n + 1;
	int TT = n + 2;

	Dinic dinic(n + 3);
	vector<long long> balance(n + 1, 0);

	auto add_bounded = [&](int u, int v, long long lower, long long upper) {
		// assume 0 <= lower <= upper
		dinic.add_edge(u, v, upper - lower);
		balance[u] -= lower;
		balance[v] += lower;
	};

	for (int i = 0; i < m; i++) {
		int u, v;
		long long lower, upper;
		cin >> u >> v >> lower >> upper;
		add_bounded(u, v, lower, upper);
	}

	// Add extra edge t -> s with [0, +inf] to turn s-t flow into circulation.
	int special_u = t;
	int special_idx = dinic.add_edge(t, s, INF);

	long long total_demand = 0;
	for (int i = 1; i <= n; i++) {
		if (balance[i] > 0) {
			dinic.add_edge(SS, i, balance[i]);
			total_demand += balance[i];
		} else if (balance[i] < 0) {
			dinic.add_edge(i, TT, -balance[i]);
		}
	}

	long long pushed = dinic.maxflow(SS, TT);
	if (pushed != total_demand) {
		cout << "please go home to sleep\n";
		return 0;
	}

	// Current feasible circulation found. Flow on special edge equals current s->t flow value.
	{
		Dinic::Edge &e = dinic.g[special_u][special_idx];
		Dinic::Edge &rev = dinic.g[e.to][e.rev];
		long long x = rev.cap;

		// For minimizing x: find how much flow can be sent from t to s through other residual edges,
		// then x can be reduced by that amount (up to x).
		disable_node(dinic, SS);
		disable_node(dinic, TT);

		// Disable the special edge in both directions during this computation.
		long long saved_forward = e.cap;
		long long saved_reverse = rev.cap;
		e.cap = 0;
		rev.cap = 0;

		long long delta = dinic.maxflow(t, s);
		long long ans = x - min(delta, x);
		cout << ans << "\n";

		// (No need to restore; program ends.)
		(void)saved_forward;
		(void)saved_reverse;
	}

	return 0;
}
