#include <bits/stdc++.h>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n)) return 0;vector<int> h(n),st;for(int&i:h)cin>>i;long long ans=0;for(int i=n-1;i>=0;--i){while(!st.empty()&&h[st.back()]<h[i])st.pop_back();if(st.empty())ans+=n-i-1;else ans+=st.back()-i-1;st.push_back(i);}cout<<ans<<endl;}