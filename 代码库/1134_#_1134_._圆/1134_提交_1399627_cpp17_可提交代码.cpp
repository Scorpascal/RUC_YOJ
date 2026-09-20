#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>
using namespace std;

static const double PI = acos(-1.0);
static const double TAU = 2.0 * PI;
static const double EPS = 1e-8;
struct P { double x, y; };
P operator+(P a,P b){return {a.x+b.x,a.y+b.y};}
P operator-(P a,P b){return {a.x-b.x,a.y-b.y};}
P operator*(P a,double k){return {a.x*k,a.y*k};}
double dotp(P a,P b){return a.x*b.x+a.y*b.y;}
double crossp(P a,P b){return a.x*b.y-a.y*b.x;}
double norm2(P a){return dotp(a,a);}
double norm(P a){return sqrt(norm2(a));}
double angnorm(double a){ while(a<0)a+=TAU; while(a>=TAU)a-=TAU; return a; }
struct C { P o; double r; };
struct Sol { P c; double r; };
struct Ev { double a; int ps=0,pe=0,fs=0,fe=0; };
static void addInterval(vector<Ev>& ev, double l, double r, bool point, bool forbidden) {
    while(l<0){l+=TAU; r+=TAU;}
    while(l>=TAU){l-=TAU; r-=TAU;}
    auto add=[&](double a,int ds,int de){ a=angnorm(a); if(a<=1e-10 || a>=TAU-1e-10) return; Ev e; e.a=a; if(point)e.ps=ds,e.pe=de; else if(forbidden)e.fs=ds,e.fe=de; ev.push_back(e); };
    if(r <= TAU + 1e-10) { if(l>1e-10) add(l,1,0); if(r<TAU-1e-10) add(r,0,1); }
    else { double rr=r-TAU; if(rr>1e-10) add(rr,0,1); if(l>1e-10) add(l,1,0); }
}
static vector<P> circleInter(P a,double ra,P b,double rb){
    vector<P> z; P d=b-a; double D=norm(d); if(D<EPS) return z;
    if(D>ra+rb+1e-7 || D<fabs(ra-rb)-1e-7) return z;
    double t=(ra*ra-rb*rb+D*D)/(2*D); double h2=ra*ra-t*t; if(h2<-1e-7) return z;
    if(h2<0)h2=0; double h=sqrt(h2); P u=d*(1.0/D), q=a+u*t; P v={-u.y,u.x}; z.push_back(q+v*h); if(h>1e-7)z.push_back(q-v*h); return z;
}
static vector<Sol> tangent3(const C& A,const C& B,const P& Pnt,double R){
    vector<Sol> out; P d1=B.o-A.o, d2=Pnt-A.o; double det=crossp(d1,d2);
    double b1=(norm2(d1)-B.r*B.r+A.r*A.r)/2.0, b2=(norm2(d2)+A.r*A.r)/2.0;
    double q1=-(B.r-A.r), q2=A.r; if(fabs(det)<1e-10) return out;
    P x0={(b1*d2.y-d1.y*b2)/det,(d1.x*b2-b1*d2.x)/det}; P x1={(q1*d2.y-d1.y*q2)/det,(d1.x*q2-q1*d2.x)/det};
    double aa=norm2(x1)-1.0, bb=2*dotp(x0,x1)-2*A.r, cc=norm2(x0)-A.r*A.r; vector<double> roots;
    if(fabs(aa)<1e-12){ if(fabs(bb)>1e-12) roots.push_back(-cc/bb); }
    else { double D=bb*bb-4*aa*cc; if(D>=-1e-8){ if(D<0)D=0; double sd=sqrt(D); roots.push_back((-bb-sd)/(2*aa)); if(sd>1e-8)roots.push_back((-bb+sd)/(2*aa)); }}
    for(double s:roots){ if(s<-1e-7 || s>R+1e-7) continue; if(s<0)s=0; P ccnt=A.o+x0+x1*s; if(fabs(norm(ccnt-A.o)-(s+A.r))>1e-5 || fabs(norm(ccnt-B.o)-(s+B.r))>1e-5 || fabs(norm(ccnt-Pnt)-s)>1e-5) continue; out.push_back({ccnt,s}); }
    return out;
}
int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr); int n,m; long long Rll; if(!(cin>>n>>m>>Rll)) return 0; double R=Rll;
    vector<C> ob(n); for(auto &c:ob){ long long x,y,r;cin>>x>>y>>r;c={{(double)x,(double)y},(double)r}; }
    vector<P> pts(m); for(auto &p:pts){long long x,y;cin>>x>>y;p={(double)x,(double)y};} int ans=0;
    auto evalDisk=[&](P cen,double rad)->int{ if(rad<-1e-7 || rad>R+1e-6) return -1; for(const auto &o:ob) if(norm(cen-o.o)+1e-7 < rad+o.r) return -1; int z=0; double rr=rad*rad+1e-7; for(const auto &p:pts) if(norm2(p-cen)<=rr) ++z; return z; };
    for(int i=0;i<m;i++){
        vector<Ev> ev; int curP=0,curF=0;
        for(int j=0;j<m;j++){ P d=pts[j]-pts[i]; double D=norm(d); if(D<1e-10){ ++curP; continue; } if(D>2*R+1e-9) continue; double ph=atan2(d.y,d.x), al=acos(max(-1.0,min(1.0,D/(2*R)))); addInterval(ev,ph-al,ph+al,true,false); }
        vector<double> special;
        for(const auto&o:ob){ P d=o.o-pts[i]; double D=norm(d), B=R+o.r; if(D<1e-10){ if(B>R+1e-9) ++curF; continue; } if(D+R < B-1e-8){ ++curF; continue; } if(D+R <= B+1e-8){ special.push_back(angnorm(atan2(-d.y,-d.x))); ++curF; continue; } if(D>=R+B-1e-8) continue; if(D<=fabs(B-R)+1e-8){ if(R>B+D+1e-8) continue; } double co=(D*D+R*R-B*B)/(2*D*R); if(co<=-1+1e-10){++curF;continue;} if(co>=1-1e-10)continue; double al=acos(max(-1.0,min(1.0,co))), ph=atan2(d.y,d.x); addInterval(ev,ph-al,ph+al,false,true); }
        ans=max(ans,evalDisk(pts[i]+P{R,0},R));
        sort(ev.begin(),ev.end(),[](const Ev&a,const Ev&b){return a.a<b.a;});
        if(ev.empty()) continue;
        size_t first=0; vector<double> ea;
        while(first<ev.size()){ double aa=ev[first].a; size_t t=first+1; while(t<ev.size()&&fabs(ev[t].a-aa)<1e-10) ++t; ea.push_back(aa); first=t; }
        double prev=ea.back()-TAU, start=(prev+ea.front())/2.0; if(start<0) start+=TAU;
        P cstart=pts[i]+P{R*cos(start),R*sin(start)}; curP=curF=0;
        for(const auto&p:pts) if(norm2(p-cstart)<=R*R+1e-7) ++curP;
        for(const auto&o:ob) if(norm(cstart-o.o)+1e-7 < R+o.r) ++curF;
        if(curF==0) ans=max(ans,curP);
        for(size_t k=0;k<ev.size();){ size_t t=k; int sp=0,ep=0,sf=0,ef=0; double aa=ev[k].a; while(t<ev.size()&&fabs(ev[t].a-aa)<1e-10){sp+=ev[t].ps;ep+=ev[t].pe;sf+=ev[t].fs;ef+=ev[t].fe;++t;} int exactP=curP+sp, exactF=curF-ef; if(exactF==0) ans=max(ans,exactP); curP+=sp-ep; curF+=sf-ef; if(curF==0) ans=max(ans,curP); k=t; }
        for(double aa:special) ans=max(ans,evalDisk(pts[i]+P{R*cos(aa),R*sin(aa)},R));
    }
    for(int i=0;i<n;i++){
        vector<double> angles; angles.push_back(0.0); auto addAngle=[&](P c){ angles.push_back(angnorm(atan2(c.y-ob[i].o.y,c.x-ob[i].o.x))); }; for(const auto&p:pts){ double d=norm(p-ob[i].o); if(fabs(d-ob[i].r)<1e-6 && d>1e-10) addAngle(p); } for(int k=i+1;k<n;k++) for(P c:circleInter(ob[i].o,ob[i].r,ob[k].o,ob[k].r)) addAngle(c); for(const auto&p:pts){ for(P c:circleInter(ob[i].o,ob[i].r+R,p,R)){ if(evalDisk(c,R)>=0) addAngle(c); } } for(int k=0;k<n;k++) if(k!=i) for(const auto&p:pts){ for(const Sol&s:tangent3(ob[i],ob[k],p,R)){ int z=evalDisk(s.c,s.r); if(z>=0) addAngle(s.c); } }
        sort(angles.begin(),angles.end()); vector<double> ua; for(double a:angles) if(ua.empty()||fabs(a-ua.back())>1e-8) ua.push_back(a); angles.swap(ua); auto evalTheta=[&](double th){ P u={cos(th),sin(th)}; double rho=ob[i].r+R; P w; for(int k=0;k<n;k++) if(k!=i){ w=ob[k].o-ob[i].o; double delta=ob[i].r-ob[k].r; double A=dotp(u,w)-delta; double D=norm2(w)-delta*delta; if(A>1e-12) rho=min(rho,D/(2*A)); } if(rho<ob[i].r-1e-7) return; P c=ob[i].o+u*rho; int z=evalDisk(c,rho-ob[i].r); if(z>=0) ans=max(ans,z); }; if(angles.size()==1){evalTheta(angles[0]); continue;} for(size_t k=0;k<angles.size();k++){ double a=angles[k]; evalTheta(a); double b=(k+1<angles.size()?angles[k+1]:angles[0]+TAU); if(b-a>1e-9) evalTheta((a+b)/2); }
    }
    cout<<ans<<'\n'; return 0;
}