#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;cin>>n;vector<int>a(n);for(int&x:a)cin>>x;int r,m;cin>>r>>m;a.insert(a.begin()+r,m);if(r>1)a.erase(a.begin()+r-2);for(size_t i=0;i<a.size();++i){if(i)cout<<' ';cout<<a[i];}cout<<endl;}