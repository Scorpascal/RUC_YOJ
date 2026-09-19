#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;unordered_set<int> seen;seen.reserve((size_t)n*2+1);bool first=true;for(int i=0,x;i<n;i++){cin>>x;if(seen.insert(x).second){if(!first)cout<<' ';first=false;cout<<x;}}cout<<endl;return 0;}