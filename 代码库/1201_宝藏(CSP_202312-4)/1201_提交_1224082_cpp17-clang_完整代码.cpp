#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
using namespace std;

static const uint32_t MOD = 998244353u;

struct Mat {
    uint32_t a11=1,a12=0,a21=0,a22=1;
    Mat() {}
    Mat(uint32_t A11,uint32_t A12,uint32_t A21,uint32_t A22):a11(A11),a12(A12),a21(A21),a22(A22){}
};
inline Mat mul(const Mat& x, const Mat& y){
    Mat r;
    uint64_t t11 = (uint64_t)x.a11*y.a11 + (uint64_t)x.a12*y.a21;
    uint64_t t12 = (uint64_t)x.a11*y.a12 + (uint64_t)x.a12*y.a22;
    uint64_t t21 = (uint64_t)x.a21*y.a11 + (uint64_t)x.a22*y.a21;
    uint64_t t22 = (uint64_t)x.a21*y.a12 + (uint64_t)x.a22*y.a22;
    r.a11 = (uint32_t)(t11 % MOD);
    r.a12 = (uint32_t)(t12 % MOD);
    r.a21 = (uint32_t)(t21 % MOD);
    r.a22 = (uint32_t)(t22 % MOD);
    return r;
}
inline Mat I(){ return Mat(1,0,0,1); }

struct Op{ int t; Mat m; };
struct Elem{ bool isL; Mat m; };

struct Block{
    int L=1,R=0;
    int need=0;
    vector<Elem> res;
    vector<Mat> prePL, prePR;
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n,m;
    if(!(cin>>n>>m)) return 0;

    vector<Op> ops(n+1);
    for(int i=1;i<=n;i++){
        int t; cin>>t;
        if(t==1||t==2){
            uint32_t a,b,c,d; cin>>a>>b>>c>>d;
            ops[i]={t, Mat(a,b,c,d)};
        }else{
            ops[i]={3, Mat()};
        }
    }

    int B = 380;
    int nb = (n + B - 1)/B;
    vector<Block> blk(nb);

    auto build_prefix = [&](Block& b){
        b.prePL.assign(1, I());
        b.prePR.assign(1, I());
        Mat PL = I(), PR = I();
        for(const auto& e : b.res){
            if(e.isL) PL = mul(e.m, PL);
            else      PR = mul(PR, e.m);
            b.prePL.push_back(PL);
            b.prePR.push_back(PR);
        }
    };

    auto rebuild_block = [&](int bid){
        Block &b = blk[bid];
        b.res.clear(); b.need=0;
        for(int i=b.L;i<=b.R;i++){
            int t = ops[i].t;
            if(t==1) b.res.push_back({true, ops[i].m});
            else if(t==2) b.res.push_back({false, ops[i].m});
            else{
                if(!b.res.empty()) b.res.pop_back();
                else b.need++;
            }
        }
        build_prefix(b);
    };

    for(int k=0;k<nb;k++){
        blk[k].L = k*B + 1;
        blk[k].R = min(n, (k+1)*B);
        rebuild_block(k);
    }

    struct Chunk{
        bool isBlock=false; int bid=-1; int len=0;
        vector<Mat> prePL, prePR; // for partial chunks
    };

    auto chunkPL = [&](const Chunk& ch)->Mat{
        return ch.isBlock ? blk[ch.bid].prePL[ch.len] : ch.prePL[ch.len];
    };
    auto chunkPR = [&](const Chunk& ch)->Mat{
        return ch.isBlock ? blk[ch.bid].prePR[ch.len] : ch.prePR[ch.len];
    };

    auto shrink_stack = [&](int k,
                             vector<Chunk>& chunks,
                             vector<Mat>& prefPL_rev,
                             vector<Mat>& prefPR){
        while(k>0 && !chunks.empty()){
            Chunk &last = chunks.back();
            if(last.len <= k){
                k -= last.len;
                chunks.pop_back();
                prefPL_rev.pop_back();
                prefPR.pop_back();
            }else{
                last.len -= k;
                Mat PL_before = prefPL_rev[prefPL_rev.size()-2];
                Mat PR_before = prefPR[prefPR.size()-2];
                Mat newPL = chunkPL(last);
                Mat newPR = chunkPR(last);
                prefPL_rev.back() = mul(newPL, PL_before);
                prefPR.back()     = mul(PR_before, newPR);
                k = 0;
            }
        }
    };

    auto build_partial_chunk = [&](int l,int r)->pair<int,Chunk>{
        int need=0;
        vector<Elem> res; res.reserve(r-l+1);
        for(int i=l;i<=r;i++){
            int t = ops[i].t;
            if(t==1) res.push_back({true, ops[i].m});
            else if(t==2) res.push_back({false, ops[i].m});
            else{
                if(!res.empty()) res.pop_back();
                else need++;
            }
        }
        Chunk ch;
        ch.isBlock=false;
        ch.len = (int)res.size();
        ch.prePL.assign(1, I());
        ch.prePR.assign(1, I());
        Mat PL=I(), PR=I();
        for(auto &e:res){
            if(e.isL) PL = mul(e.m, PL);
            else      PR = mul(PR, e.m);
            ch.prePL.push_back(PL);
            ch.prePR.push_back(PR);
        }
        return {need, ch};
    };

    auto append_chunk = [&](const Chunk& ch,
                            vector<Chunk>& chunks,
                            vector<Mat>& prefPL_rev,
                            vector<Mat>& prefPR){
        if(ch.len==0) return;
        Mat PL_before = prefPL_rev.back();
        Mat PR_before = prefPR.back();
        Mat addPL = chunkPL(ch);
        Mat addPR = chunkPR(ch);
        prefPL_rev.push_back(mul(addPL, PL_before));
        prefPR.push_back(mul(PR_before, addPR));
        chunks.push_back(ch);
    };

    auto query_range = [&](int l,int r){
        vector<Chunk> chunks;
        vector<Mat> prefPL_rev(1, I());
        vector<Mat> prefPR(1, I());

        int bl = (l-1)/B, br = (r-1)/B;
        if(bl==br){
            auto tmp = build_partial_chunk(l, r);
            int needP = tmp.first; Chunk ch = std::move(tmp.second);
            shrink_stack(needP, chunks, prefPL_rev, prefPR);
            append_chunk(ch, chunks, prefPL_rev, prefPR);
        }else{
            auto tmpL = build_partial_chunk(l, blk[bl].R);
            shrink_stack(tmpL.first, chunks, prefPL_rev, prefPR);
            append_chunk(tmpL.second, chunks, prefPL_rev, prefPR);
            for(int b=bl+1;b<=br-1;b++){
                shrink_stack(blk[b].need, chunks, prefPL_rev, prefPR);
                if(!blk[b].res.empty()){
                    Chunk ch; ch.isBlock=true; ch.bid=b; ch.len=(int)blk[b].res.size();
                    append_chunk(ch, chunks, prefPL_rev, prefPR);
                }
            }
            auto tmpR = build_partial_chunk(blk[br].L, r);
            shrink_stack(tmpR.first, chunks, prefPL_rev, prefPR);
            append_chunk(tmpR.second, chunks, prefPL_rev, prefPR);
        }
        Mat C = mul(prefPL_rev.back(), prefPR.back());
        cout<<C.a11<<' '<<C.a12<<' '<<C.a21<<' '<<C.a22<<"\n";
    };

    for(int qi=0; qi<m; ++qi){
        int tp; cin>>tp;
        if(tp==1){
            int i, t; cin>>i>>t;
            if(t==1 || t==2){
                uint32_t a,b,c,d; cin>>a>>b>>c>>d;
                ops[i] = {t, Mat(a,b,c,d)};
            }else{
                ops[i] = {3, Mat()};
            }
            int bid = (i-1)/B;
            rebuild_block(bid);
        }else{
            int l,r; cin>>l>>r;
            query_range(l,r);
        }
    }
    return 0;
}