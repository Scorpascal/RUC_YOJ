#include <bits/stdc++.h>
using namespace std;
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    if(!(cin>>n)) return 0;
    vector<vector<long long>> a(n, vector<long long>(n));
    for(int i=0;i<n;i++) for(int j=0;j<n;j++) cin>>a[i][j];
    vector<int> p(n);
    for(int i=0;i<n;i++) p[i]=i;
    long long det = 0;
    do{
        long long prod = 1;
        for(int i=0;i<n;i++) prod *= a[i][p[i]];
        int inv = 0;
        for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) if(p[i]>p[j]) ++inv;
        if(inv%2==0) det += prod; else det -= prod;
    } while(next_permutation(p.begin(), p.end()));
    cout<<det<<"\n";
    return 0;
}