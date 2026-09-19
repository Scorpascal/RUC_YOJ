#include <bits/stdc++.h>
using namespace std;
int main(){
 ios::sync_with_stdio(false);cin.tie(nullptr);
 int n,m;if(!(cin>>n>>m)) return 0;
 vector<vector<long long>> a(m,vector<long long>(n));
 for(int r=0;r<m;r++) for(int c=0;c<n;c++) cin>>a[r][c];
 const long long NEG=-(1LL<<60); vector<long long> dp(n,NEG);
 for(int r=0;r<m;r++){
  long long diag=NEG;
  for(int c=0;c<n;c++){
   long long up=dp[c];
   if(a[r][c]<0) dp[c]=NEG;
   else if(r==0 && c==0) dp[c]=a[r][c];
   else{
    long long best=max(up,diag);
    if(c) best=max(best,dp[c-1]);
    dp[c]=(best==NEG?NEG:best+a[r][c]);
   }
   diag=up;
  }
 }
 if(dp[n-1]==NEG) cout<<"No"<<endl; else cout<<"Yes"<<endl<<dp[n-1]<<endl;
 return 0;
}