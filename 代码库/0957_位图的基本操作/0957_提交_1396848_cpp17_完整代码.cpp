#include <bits/stdc++.h>
using namespace std;int main(){int a,b,p,q;cin>>a>>b>>p>>q;unordered_set<int>s;s.insert(a);s.insert(b);s.erase(p);cout<<(s.count(q)?1:0)<<endl;}