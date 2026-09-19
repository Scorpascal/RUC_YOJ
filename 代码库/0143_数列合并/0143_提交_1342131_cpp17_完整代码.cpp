#include <bits/stdc++.h>
using namespace std;

int N;
long long target;
vector<long long> coeff;
vector<int> perm;
vector<int> used;
bool found = false;

void dfs(int pos, long long cur){
    if(found) return;
    if(pos==N){
        if(cur==target){
            for(int i=0;i<N;i++){
                if(i) cout<<' ';
                cout<<perm[i];
            }
            cout<<"\n";
            found = true;
        }
        return;
    }
    for(int v=1; v<=N; ++v){
        if(used[v]) continue;
        used[v]=1;
        perm[pos]=v;
        dfs(pos+1, cur + coeff[pos]*v);
        used[v]=0;
        if(found) return;
    }
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if(!(cin>>N>>target)) return 0;
    coeff.assign(N,0);
    // compute C(N-1, i)
    vector<vector<long long>> C(N, vector<long long>(N,0));
    for(int i=0;i<N;i++){
        C[i][0]=C[i][i]=1;
        for(int j=1;j<i;j++) C[i][j]=C[i-1][j-1]+C[i-1][j];
    }
    for(int i=0;i<N;i++) coeff[i]=C[N-1][i];
    perm.assign(N,0);
    used.assign(N+1,0);
    dfs(0,0);
    return 0;
}