#include <iostream>
#include <vector>
using namespace std;
static int step_value(int value,int k){int a=(value>>6)&7,b=(value>>3)&7,c=value&7;auto f=[k](int x){return (((x*x+k*k)&7)^k);};return (b<<6)|((c^f(b))<<3)|(a^f(c));}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,m;if(!(cin>>n>>m))return 0;vector<int> k(m);for(int &x:k)cin>>x;vector<int> inv(512,-1);for(int s=0;s<512;s++){int v=s;for(int x:k)v=step_value(v,x);inv[v]=s;}for(int i=0;i<n;i++){int v;cin>>v;if(i)cout<<' ';cout<<inv[v];}cout<<'\n';}