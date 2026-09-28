#include <iostream>
using namespace std;
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;int ans=0;for(int i=0;i<n;i++){unsigned int x;cin>>x;int o=0,z=0;while(x){if(x&1U)++o;else ++z;x>>=1;}if(o==z)++ans;}cout<<ans<<'\n';}