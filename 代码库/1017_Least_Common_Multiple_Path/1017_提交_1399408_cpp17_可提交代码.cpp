#include <bits/stdc++.h>
using namespace std;
struct E{int v,a,b;};
int main(){
 ios::sync_with_stdio(false);cin.tie(nullptr);
 int n,m;if(!(cin>>n>>m)) return 0;
 vector<vector<E>> g(n);
 for(int i=0;i<m;i++){
  int u,v; unsigned long long w; cin>>u>>v>>w;
  int a=0,b=0; while((w&1ULL)==0){w>>=1; a++;} while(w%3ULL==0){w/=3ULL; b++;}
  g[u].push_back({v,a,b}); g[v].push_back({u,a,b});
 }
 int s,t,x,y;cin>>s>>t>>x>>y;
 vector<array<char,4>> vis(n); queue<pair<int,int>> q;
 vis[s][0]=1; q.push({s,0});
 while(!q.empty()){
  auto [u,mask]=q.front();q.pop();
  if(u==t && mask==3){cout<<"True"<<endl;return 0;}
  for(auto e:g[u]){
   if(e.a>x || e.b>y) continue;
   int nm=mask | (e.a==x?1:0) | (e.b==y?2:0);
   if(!vis[e.v][nm]){vis[e.v][nm]=1;q.push({e.v,nm});}
  }
 }
 cout<<"False"<<endl;
 return 0;
}