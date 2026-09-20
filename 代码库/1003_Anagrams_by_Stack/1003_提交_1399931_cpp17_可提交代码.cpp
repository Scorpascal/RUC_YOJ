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
string s,t,ops; vector<char> st;
void dfs(int p,int q){
    if(p==(int)s.size() && q==(int)t.size() && st.empty()){
        for(char c:ops) cout<<c<<' ';
        cout<<endl; return;
    }
    if(p<(int)s.size()){
        st.push_back(s[p]); ops.push_back('i'); dfs(p+1,q); ops.pop_back(); st.pop_back();
    }
    if(!st.empty() && q<(int)t.size() && st.back()==t[q]){
        char c=st.back(); st.pop_back(); ops.push_back('o'); dfs(p,q+1); ops.pop_back(); st.push_back(c);
    }
}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);string a,b;while(getline(cin,a)){if(!getline(cin,b)) break; if(!a.empty()&&a.back()=='\r') a.pop_back(); if(!b.empty()&&b.back()=='\r') b.pop_back(); s=a;t=b;ops.clear();st.clear();cout<<"["<<endl;dfs(0,0);cout<<"]"<<endl;}return 0;}