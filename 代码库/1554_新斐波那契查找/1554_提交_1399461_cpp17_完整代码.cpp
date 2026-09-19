#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,x;if(!(cin>>n>>x))return 0;vector<int>a(n);for(int&v:a)cin>>v;auto found=lower_bound(a.begin(),a.end(),x);if(found==a.end()||*found!=x){cout<<-1<<char(10);return 0;}vector<int>f{1,1};while(f.back()<n)f.push_back(f.back()+f[f.size()-2]);int k=f.size()-1,l=0,r=n;while(l<r){while(f[k]>r-l)--k;int mid=l+f[k]-1;if(a[mid]<x){cout<<a[mid]<<' ';l=mid+1;}else r=mid;}cout<<x<<char(10);}