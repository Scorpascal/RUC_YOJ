#include <bits/stdc++.h>
using namespace std;
vector<string> t; int at=0; long long ans=0;
long long go(){string s=t[at++]; if(s=="#") return 0; long long v=stoll(s),l=go(),r=go(); ans=max(ans,v+max(0LL,l)+max(0LL,r)); return v+max(0LL,max(l,r));}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);string s;if(!(cin>>s))return 0;string u;for(char c:s){if(c==','){t.push_back(u);u.clear();}else u.push_back(c);}if(!u.empty())t.push_back(u);go();cout<<ans<<endl;return 0;}