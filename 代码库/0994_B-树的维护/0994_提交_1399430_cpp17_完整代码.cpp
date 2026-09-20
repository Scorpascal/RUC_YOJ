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
using namespace std;struct Node{int k,c,sz;unsigned pri;Node*l,*r;Node(int v,unsigned p):k(v),c(1),sz(1),pri(p),l(nullptr),r(nullptr){}};unsigned seedv=712367821;unsigned rnd(){seedv^=seedv<<13;seedv^=seedv>>17;seedv^=seedv<<5;return seedv;}
int sz(Node*t){return t?t->sz:0;}void up(Node*t){if(t)t->sz=t->c+sz(t->l)+sz(t->r);}Node*rotR(Node*t){Node*u=t->l;t->l=u->r;u->r=t;up(t);up(u);return u;}Node*rotL(Node*t){Node*u=t->r;t->r=u->l;u->l=t;up(t);up(u);return u;}
Node*ins(Node*t,int x){if(!t)return new Node(x,rnd());if(x==t->k)t->c++;else if(x<t->k){t->l=ins(t->l,x);if(t->l->pri>t->pri)t=rotR(t);}else{t->r=ins(t->r,x);if(t->r->pri>t->pri)t=rotL(t);}up(t);return t;}
Node*del(Node*t,int x){if(!t)return t;if(x<t->k)t->l=del(t->l,x);else if(x>t->k)t->r=del(t->r,x);else if(t->c>1)t->c--;else{if(!t->l){Node*u=t->r;delete t;return u;}if(!t->r){Node*u=t->l;delete t;return u;}if(t->l->pri>t->r->pri){t=rotR(t);t->r=del(t->r,x);}else{t=rotL(t);t->l=del(t->l,x);}}up(t);return t;}
int rankx(Node*t,int x){int r=1;while(t){if(x<=t->k)t=t->l;else{r+=sz(t->l)+t->c;t=t->r;}}return r;}int kth(Node*t,int k){while(t){if(k<=sz(t->l))t=t->l;else if(k<=sz(t->l)+t->c)return t->k;else{k-=sz(t->l)+t->c;t=t->r;}}return 0;}int pred(Node*t,int x){int a=INT_MIN;while(t){if(t->k<x){a=t->k;t=t->r;}else t=t->l;}return a;}int succ(Node*t,int x){int a=INT_MAX;while(t){if(t->k>x){a=t->k;t=t->l;}else t=t->r;}return a;}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;Node*root=nullptr;string out;while(n--){int op,x;cin>>op>>x;if(op==1)root=ins(root,x);else if(op==2)root=del(root,x);else if(op==3){out+=to_string(rankx(root,x));out.push_back(10);}else if(op==4){out+=to_string(kth(root,x));out.push_back(10);}else if(op==5){out+=to_string(pred(root,x));out.push_back(10);}else if(op==6){out+=to_string(succ(root,x));out.push_back(10);}}cout<<out;}