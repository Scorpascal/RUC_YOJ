#include <bits/stdc++.h>
using namespace std;
int main(){
 ios::sync_with_stdio(false); cin.tie(nullptr);
 int n; const long long INF=4000000000000000000LL;
 while(cin>>n && n){
  vector<vector<long long>> d(n+1,vector<long long>(n+1,INF));
  vector<vector<vector<int>>> p(n+1,vector<vector<int>>(n+1));
  for(int i=1;i<=n;i++) for(int j=1;j<=n;j++){
   long long w; cin>>w;
   if(i==j){d[i][j]=0; p[i][j]={i};}
   else if(w>=0){d[i][j]=w; p[i][j]={i,j};}
  }
  vector<long long> tax(n+1); for(int i=1;i<=n;i++) cin>>tax[i];
  for(int k=1;k<=n;k++) for(int i=1;i<=n;i++) if(i!=k && d[i][k]<INF)
   for(int j=1;j<=n;j++) if(j!=k && i!=j && d[k][j]<INF){
    long long nd=d[i][k]+d[k][j]+tax[k];
    vector<int> q=p[i][k]; q.insert(q.end(),p[k][j].begin()+1,p[k][j].end());
    if(nd<d[i][j] || (nd==d[i][j] && q<p[i][j])){d[i][j]=nd; p[i][j]=q;}
   }
  int s,t;
  while(cin>>s>>t){
   if(s==-1 && t==-1) break;
   if(d[s][t]>=INF){cout<<-1<<endl<<-1<<endl; continue;}
   for(size_t i=0;i<p[s][t].size();i++){if(i) cout<<"->"; cout<<p[s][t][i];}
   cout<<endl<<d[s][t]<<endl;
  }
 }
 return 0;
}