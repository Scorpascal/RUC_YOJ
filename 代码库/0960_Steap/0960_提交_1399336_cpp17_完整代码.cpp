#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n)) return 0;vector<int>s,m;while(n--){int op;cin>>op;if(op==0){s.pop_back();m.pop_back();}else if(op==1){int x;cin>>x;s.push_back(x);m.push_back(m.empty()?x:max(m.back(),x));}else if(op==2){cout<<m.back()<<endl;}}return 0;}