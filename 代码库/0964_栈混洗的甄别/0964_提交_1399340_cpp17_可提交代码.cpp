#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;vector<int> p(n),q(n);for(int&i:p)cin>>i;for(int&i:q)cin>>i;vector<int>s;int j=0;for(int x:q){while(j<n&&(s.empty()||s.back()!=x))s.push_back(p[j++]);if(s.empty()||s.back()!=x){cout<<"No"<<endl;return 0;}s.pop_back();}cout<<"Yes"<<endl;return 0;}