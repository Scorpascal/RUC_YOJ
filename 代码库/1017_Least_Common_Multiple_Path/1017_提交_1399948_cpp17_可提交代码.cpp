#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
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