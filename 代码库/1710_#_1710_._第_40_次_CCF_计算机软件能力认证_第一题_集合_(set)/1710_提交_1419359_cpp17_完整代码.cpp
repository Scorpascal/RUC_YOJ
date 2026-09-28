#include <iostream>
#include <vector>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,m;if(!(cin>>n>>m)) return 0;vector<int>a(n);for(int &x:a)cin>>x;vector<vector<int>>s(m),t(m);vector<int>sx(m),tx(m);for(int i=0;i<m;i++){int z;cin>>z;s[i].resize(z);for(int &x:s[i]){cin>>x;sx[i]^=a[x-1];}}for(int i=0;i<m;i++){int z;cin>>z;t[i].resize(z);for(int &x:t[i]){cin>>x;tx[i]^=a[x-1];}}for(int i=0;i<m;i++) cout<<((s[i]==t[i])==(sx[i]==tx[i])?"correct":"wrong")<<"\n";}