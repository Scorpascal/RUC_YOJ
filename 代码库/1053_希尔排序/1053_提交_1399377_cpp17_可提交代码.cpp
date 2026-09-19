#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;vector<long long>a(n);for(auto &x:a)cin>>x;int gap=n/2;for(int i=gap;i<n;i++){long long x=a[i];int j=i;while(j>=gap&&a[j-gap]>x){a[j]=a[j-gap];j-=gap;}a[j]=x;}for(int i=0;i<n;i++){if(i)cout<<' ';cout<<a[i];}cout<<endl;return 0;}