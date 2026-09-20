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