#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
using namespace std;
const int DMAX=32;
struct Op{int type,l,r;long long v;};
struct Info{int s0=0,s00=0,sp[DMAX]{},sp0[DMAX]{};};
struct Seg{
 int n=0,d=1;long long k=0;vector<int>s0,s00;vector<vector<int>>sp,sp0,lz;vector<long long>a;
  int md(__int128 x)const{x%=k;if(x<0)x+=k;return(int)x;}int m(long long x)const{return md(x);} 
   void split(long long x,int o[DMAX])const{for(int i=0;i<DMAX;i++)o[i]=0;for(int i=0;i<d;i++){o[i]=x%k;x/=k;}}
    void apply(int p,int len,const int c[DMAX]){int o0=s0[p],o00=s00[p],c0=c[0];long long lm=len%k;s0[p]=m(o0+(__int128)lm*c0);s00[p]=md((__int128)o00+2LL*c0*o0+(__int128)lm*c0*c0);for(int q=1;q<d;q++){int op=sp[q][p],op0=sp0[q][p],cq=c[q];sp[q][p]=m(op+(__int128)lm*cq);sp0[q][p]=md((__int128)op0+1LL*c0*op+1LL*cq*o0+(__int128)lm*cq*c0);}for(int q=0;q<d;q++)lz[q][p]=m(lz[q][p]+c[q]);}
     void pull(int p){int x=p<<1,y=x|1;s0[p]=m((long long)s0[x]+s0[y]);s00[p]=m((long long)s00[x]+s00[y]);for(int q=1;q<d;q++){sp[q][p]=m((long long)sp[q][x]+sp[q][y]);sp0[q][p]=m((long long)sp0[q][x]+sp0[q][y]);}}
      void build(int p,int l,int r){if(l==r){int x[DMAX];split(a[l],x);s0[p]=x[0];s00[p]=m(1LL*x[0]*x[0]);for(int q=1;q<d;q++){sp[q][p]=x[q];sp0[q][p]=m(1LL*x[q]*x[0]);}return;}int mid=(l+r)>>1;build(p<<1,l,mid);build(p<<1|1,mid+1,r);pull(p);}
       void push(int p,int l,int r){if(l==r)return;bool ok=0;int c[DMAX];for(int q=0;q<d;q++){c[q]=lz[q][p];ok|=c[q]!=0;}if(!ok)return;int mid=(l+r)>>1;apply(p<<1,mid-l+1,c);apply(p<<1|1,r-mid,c);for(int q=0;q<d;q++)lz[q][p]=0;}
        void upd(int p,int l,int r,int ql,int qr,const int c[DMAX]){if(ql<=l&&r<=qr){apply(p,r-l+1,c);return;}push(p,l,r);int mid=(l+r)>>1;if(ql<=mid)upd(p<<1,l,mid,ql,qr,c);if(qr>mid)upd(p<<1|1,mid+1,r,ql,qr,c);pull(p);}
         Info node(int p)const{Info x;x.s0=s0[p];x.s00=s00[p];for(int q=1;q<d;q++){x.sp[q]=sp[q][p];x.sp0[q]=sp0[q][p];}return x;}
          Info merge(const Info&a,const Info&b)const{Info x;x.s0=m((long long)a.s0+b.s0);x.s00=m((long long)a.s00+b.s00);for(int q=1;q<d;q++){x.sp[q]=m((long long)a.sp[q]+b.sp[q]);x.sp0[q]=m((long long)a.sp0[q]+b.sp0[q]);}return x;}
           Info qry(int p,int l,int r,int ql,int qr){if(ql<=l&&r<=qr)return node(p);push(p,l,r);int mid=(l+r)>>1;if(qr<=mid)return qry(p<<1,l,mid,ql,qr);if(ql>mid)return qry(p<<1|1,mid+1,r,ql,qr);return merge(qry(p<<1,l,mid,ql,qr),qry(p<<1|1,mid+1,r,ql,qr));}
            void init(vector<long long>v,long long base,int dig){a=move(v);n=a.size()-1;k=base;d=dig;int N=4*n+8;s0.assign(N,0);s00.assign(N,0);sp.assign(d,vector<int>(N));sp0.assign(d,vector<int>(N));lz.assign(d,vector<int>(N));build(1,1,n);}
             void update(int l,int r,long long v){int c[DMAX];split(v,c);upd(1,1,n,l,r,c);}Info query(int l,int r){return qry(1,1,n,l,r);}
              void answer(const Info&x){long long inv=(k+1)/2;int low=m((long long)x.s00+x.s0);low=m(1LL*low*inv);__int128 ans=low,p=k;for(int q=1;q<d;q++){ans+=p*m((long long)x.sp0[q]+x.sp[q]);p*=k;}if(ans==0){cout<<0<<'\n';return;}string z;while(ans){z.push_back(char('0'+ans%10));ans/=10;}reverse(z.begin(),z.end());cout<<z<<'\n';}
              };
              int main(){ios::sync_with_stdio(false);cin.tie(nullptr);int n,mq;long long k;if(!(cin>>n>>mq>>k))return 0;vector<long long>a(n+1);long long mx=0;for(int i=1;i<=n;i++){cin>>a[i];mx=max(mx,a[i]);}vector<Op>o(mq);for(auto &x:o){cin>>x.type>>x.l>>x.r;if(x.type==1){cin>>x.v;mx=max(mx,x.v);}else x.v=0;}int d=1;for(long long x=mx;x>=k;x/=k)++d;Seg st;st.init(move(a),k,d);for(auto &x:o){if(x.type==1)st.update(x.l,x.r,x.v);else st.answer(st.query(x.l,x.r));}}