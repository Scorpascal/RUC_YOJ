#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
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