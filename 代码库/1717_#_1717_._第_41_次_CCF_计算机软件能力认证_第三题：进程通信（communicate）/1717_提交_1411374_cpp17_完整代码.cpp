#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <limits>
using namespace std;using ll=long long;
struct Q{ll l,r,nxt;};
struct Mem{map<ll,ll> L;set<pair<ll,ll>> S;void ins(ll l,ll r){L[l]=r;S.insert({r-l+1,l});}void era(map<ll,ll>::iterator it){ll l=it->first,r=it->second;S.erase({r-l+1,l});L.erase(it);}Mem(){ins(0,4000000000000000000LL);}pair<ll,ll> alloc(ll len){auto q=S.lower_bound({len,numeric_limits<ll>::min()});auto it=L.find(q->second);ll l=it->first,r=it->second;era(it);ll e=l+len-1;if(e<r)ins(e+1,r);return{l,e};}void freeit(ll l,ll r){auto it=L.lower_bound(l);if(it!=L.begin()){auto p=prev(it);if(p->second+1==l){l=p->first;r=max(r,p->second);era(p);}}it=L.lower_bound(l);if(it!=L.end()&&r+1==it->first){r=max(r,it->second);era(it);}ins(l,r);}};
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,q;if(!(cin>>n>>q))return 0;vector<vector<Q>>p(n+1);Mem mem;while(q--){string op;cin>>op;if(op=="new"){int x;ll len;cin>>x>>len;auto [l,r]=mem.alloc(len);p[x].push_back({l,r,l});cout<<l<<'\n';}else if(op=="send"){int x;cin>>x;ll ans=0;for(auto &v:p[x]){ans+=v.nxt;v.nxt=v.nxt==v.r?v.l:v.nxt+1;}cout<<ans<<'\n';}else{int x,i;cin>>x>>i;--i;auto v=p[x][i];mem.freeit(v.l,v.r);p[x].erase(p[x].begin()+i);}}}