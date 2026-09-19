#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <tuple>
#include <limits>
#include <cstring>
using namespace std;
using ll = long long;

struct Edge {
	int to;
	ll cap;
	int rev;
	ll orig;
};

struct Dinic {
	int n;
	vector<vector<Edge>> g;
	vector<int> level, it;
	Dinic(int n=0): n(n), g(n+1), level(n+1), it(n+1) {}
	void reset(int N){ n=N; g.assign(n+1,{}); level.assign(n+1,0); it.assign(n+1,0); }
	pair<int,int> addEdge(int u,int v,ll c){
		Edge a{v,c,(int)g[v].size(),c};
		Edge b{u,0,(int)g[u].size(),0};
		g[u].push_back(a);
		g[v].push_back(b);
		return {u,(int)g[u].size()-1};
	}
	bool bfs(int s,int t){
		fill(level.begin(), level.end(), -1);
		queue<int> q; level[s]=0; q.push(s);
		while(!q.empty()){
			int u=q.front(); q.pop();
			for(auto &e: g[u]) if(e.cap>0 && level[e.to]==-1){ level[e.to]=level[u]+1; q.push(e.to);} 
		}
		return level[t]!=-1;
	}
	ll dfs(int u,int t,ll f){
		if(u==t) return f;
		for(int &i=it[u]; i<(int)g[u].size(); ++i){
			Edge &e = g[u][i];
			if(e.cap>0 && level[e.to]==level[u]+1){
				ll ret = dfs(e.to, t, min(f, e.cap));
				if(ret>0){ e.cap -= ret; g[e.to][e.rev].cap += ret; return ret; }
			}
		}
		return 0;
	}
	ll maxflow(int s,int t){
		ll flow=0;
		while(bfs(s,t)){
			fill(it.begin(), it.end(), 0);
			while(true){ ll f = dfs(s,t, (ll)9e18 ); if(!f) break; flow += f; }
		}
		return flow;
	}
};

int main(){
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n,m,s,t;
	if(!(cin>>n>>m>>s>>t)) return 0;
	int Ssup = n+1, Tsup = n+2;
	Dinic din(n+2);
	vector<ll> demand(n+3,0);
	const ll INF = (ll)4e18;
	vector<tuple<int,int,ll,ll>> edges;
	edges.reserve(m);
	for(int i=0;i<m;i++){
		int u,v; ll low, up; cin>>u>>v>>low>>up;
		edges.emplace_back(u,v,low,up);
		demand[u] -= low;
		demand[v] += low;
		din.addEdge(u,v, up - low);
	}
	auto idx_ts = din.addEdge(t, s, INF);

	ll need=0;
	for(int i=1;i<=n;i++){
		if(demand[i]>0){ din.addEdge(Ssup, i, demand[i]); need += demand[i]; }
		else if(demand[i]<0) din.addEdge(i, Tsup, -demand[i]);
	}

	ll f = din.maxflow(Ssup, Tsup);
	if(f != need){
		cout<<"please go home to sleep\n";
		return 0;
	}

	int from = idx_ts.first, pos = idx_ts.second;
	ll used_on_ts = din.g[from][pos].orig - din.g[from][pos].cap;

	int to = din.g[from][pos].to;
	int rev = din.g[from][pos].rev;
	din.g[from][pos].cap = 0;
	din.g[to][rev].cap = 0;

	for(int u=0; u<=din.n; ++u){
		for(auto &e: din.g[u]){
			if(e.to==Ssup || e.to==Tsup || u==Ssup || u==Tsup){ e.cap = 0; }
		}
	}

	ll add = din.maxflow(s, t);
	cout << (used_on_ts + add) << '\n';
	return 0;
}
