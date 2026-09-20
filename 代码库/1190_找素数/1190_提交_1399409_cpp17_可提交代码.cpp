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