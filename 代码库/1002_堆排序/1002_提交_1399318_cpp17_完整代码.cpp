#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n)) return 0; priority_queue<int,vector<int>,greater<int>> q; for(int i=0,x;i<n;i++){cin>>x;q.push(x);} for(int i=0;i<n;i++){if(i) cout<<' '; cout<<q.top();q.pop();} cout<<endl;}