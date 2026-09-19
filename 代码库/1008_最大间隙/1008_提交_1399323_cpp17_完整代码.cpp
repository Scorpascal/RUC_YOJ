#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;vector<long long>a(n);for(auto&x:a)cin>>x;sort(a.begin(),a.end());long long ans=0;for(int i=1;i<n;i++)ans=max(ans,a[i]-a[i-1]);cout<<ans<<endl;}