#include <bits/stdc++.h>
using namespace std;
int main(){
 ios::sync_with_stdio(false);cin.tie(nullptr);
 int n;if(!(cin>>n)) return 0;
 vector<bool> comp((size_t)n/2+1,false);
 string out; out.reserve((size_t)n/2);
 if(n>=2){out.push_back('2');out.push_back(10);}
 for(long long i=3;i*i<=n;i+=2) if(!comp[(size_t)i>>1])
  for(long long j=i*i;j<=n;j+=2*i) comp[(size_t)j>>1]=true;
 for(int i=3;i<=n;i+=2) if(!comp[(size_t)i>>1]){out+=to_string(i);out.push_back(10);}
 cout.write(out.data(),(streamsize)out.size());
 return 0;
}