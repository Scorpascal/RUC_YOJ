#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,x;if(!(cin>>n>>x))return 0;int l=0,r=n;vector<int>a(n);for(int&i:a)cin>>i;while(l<r){int m=l+(r-l)/2;if(a[m]<x)l=m+1;else r=m;}if(l==n||a[l]!=x)cout<<-1<<endl;else cout<<l+1<<endl;return 0;}