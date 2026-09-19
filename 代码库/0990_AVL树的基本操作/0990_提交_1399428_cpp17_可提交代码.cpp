#include <bits/stdc++.h>
using namespace std;struct Node{int k,h;Node*l,*r;Node(int v):k(v),h(1),l(nullptr),r(nullptr){}};
int ht(Node*t){return t?t->h:0;}void up(Node*t){t->h=1+max(ht(t->l),ht(t->r));}int bf(Node*t){return ht(t->l)-ht(t->r);}
Node*rr(Node*y){Node*x=y->l,*z=x->r;x->r=y;y->l=z;up(y);up(x);return x;}Node*rl(Node*x){Node*y=x->r,*z=y->l;y->l=x;x->r=z;up(x);up(y);return y;}
Node*bal(Node*t){if(!t)return t;up(t);if(bf(t)>1){if(bf(t->l)<0)t->l=rl(t->l);return rr(t);}if(bf(t)<-1){if(bf(t->r)>0)t->r=rr(t->r);return rl(t);}return t;}
Node*ins(Node*t,int x){if(!t)return new Node(x);if(x<t->k)t->l=ins(t->l,x);else if(x>t->k)t->r=ins(t->r,x);else return t;return bal(t);}
Node*mn(Node*t){while(t->l)t=t->l;return t;}Node*del(Node*t,int x){if(!t)return t;if(x<t->k)t->l=del(t->l,x);else if(x>t->k)t->r=del(t->r,x);else{if(!t->l||!t->r){Node*u=t->l?t->l:t->r;if(!u){delete t;return nullptr;}*t=*u;delete u;}else{Node*u=mn(t->r);t->k=u->k;t->r=del(t->r,u->k);}}return bal(t);}
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n;if(!(cin>>n))return 0;Node*root=nullptr;while(n--){int a,b;cin>>a>>b;if(a==1)root=ins(root,b);else root=del(root,b);}vector<int>v;function<void(Node*)>go=[&](Node*t){if(!t)return;v.push_back(t->k);go(t->l);go(t->r);};go(root);string out;for(size_t i=0;i<v.size();i++){if(i)out.push_back(' ');out+=to_string(v[i]);}cout<<out<<endl;}