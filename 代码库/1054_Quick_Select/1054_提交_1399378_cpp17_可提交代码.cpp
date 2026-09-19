#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,k;if(!(cin>>n>>k))return 0;vector<int>a(n);for(int &x:a)cin>>x;nth_element(a.begin(),a.begin()+k-1,a.end());cout<<a[k-1]<<endl;return 0;}