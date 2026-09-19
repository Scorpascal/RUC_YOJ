#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,W;if(!(cin>>n>>W))return 0;vector<long long>dp(W+1);for(int i=0,w,v;i<n;i++){cin>>w>>v;for(int j=W;j>=w;j--)dp[j]=max(dp[j],dp[j-w]+v);}cout<<dp[W]<<endl;return 0;}