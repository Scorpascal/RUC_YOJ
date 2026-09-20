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
using namespace std;using ll=long long;
struct P{ll x,y;};struct N{int l=-1,r=-1;ll mnx,mxx,mny,mxy;};
vector<P> p;vector<N> tr;
int build(int L,int R,int dep){if(L>=R)return -1;int M=(L+R)/2,ax=dep&1;nth_element(p.begin()+L,p.begin()+M,p.begin()+R,[ax](const P&a,const P&b){return ax?a.y<b.y:a.x<b.x;});int u=M;tr[u].mnx=tr[u].mxx=p[u].x;tr[u].mny=tr[u].mxy=p[u].y;tr[u].l=build(L,M,dep+1);tr[u].r=build(M+1,R,dep+1);for(int v:{tr[u].l,tr[u].r})if(v>=0){tr[u].mnx=min(tr[u].mnx,tr[v].mnx);tr[u].mxx=max(tr[u].mxx,tr[v].mxx);tr[u].mny=min(tr[u].mny,tr[v].mny);tr[u].mxy=max(tr[u].mxy,tr[v].mxy);}return u;}
ll d2(const P&a,const P&b){ll dx=a.x-b.x,dy=a.y-b.y;__int128 z=(__int128)dx*dx+(__int128)dy*dy;return (ll)z;}
ll box(int u,const P&q){if(u<0)return LLONG_MAX;ll dx=0,dy=0;if(q.x<tr[u].mnx)dx=tr[u].mnx-q.x;else if(q.x>tr[u].mxx)dx=q.x-tr[u].mxx;if(q.y<tr[u].mny)dy=tr[u].mny-q.y;else if(q.y>tr[u].mxy)dy=q.y-tr[u].mxy;return (ll)((__int128)dx*dx+(__int128)dy*dy);}
void ask(int u,const P&q,ll&best){if(u<0||box(u,q)>=best)return;best=min(best,d2(p[u],q));int a=tr[u].l,b=tr[u].r;if(box(a,q)>box(b,q))swap(a,b);ask(a,q,best);ask(b,q,best);}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;p.resize(n);for(auto&z:p)cin>>z.x>>z.y;tr.resize(n);int root=build(0,n,0);int q;cin>>q;string out;while(q--){P z;cin>>z.x>>z.y;ll best=LLONG_MAX;ask(root,z,best);out+=to_string(best);out.push_back(10);}cout<<out;}