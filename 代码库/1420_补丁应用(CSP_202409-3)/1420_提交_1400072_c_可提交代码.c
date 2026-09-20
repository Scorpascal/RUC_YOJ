#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>  // ssize_t

typedef struct {
    char **data;
    int size;
    int cap;
} StrVec;

static void sv_init(StrVec *v) { v->data=NULL; v->size=0; v->cap=0; }
static void sv_reserve(StrVec *v, int cap) {
    if (cap > v->cap) {
        int ncap = v->cap? v->cap:8;
        while (ncap < cap) ncap <<= 1;
        v->data = (char**)realloc(v->data, ncap * sizeof(char*));
        v->cap = ncap;
    }
}
static void sv_push(StrVec *v, char *s) { sv_reserve(v, v->size+1); v->data[v->size++] = s; }
static void sv_insert_range(StrVec *v, int pos, char **arr, int cnt) {
    if (cnt<=0) return;
    sv_reserve(v, v->size + cnt);
    memmove(v->data + pos + cnt, v->data + pos, (v->size - pos) * sizeof(char*));
    for (int i=0;i<cnt;i++) v->data[pos+i] = arr[i];
    v->size += cnt;
}
static void sv_erase_range(StrVec *v, int pos, int cnt, int free_str) {
    if (cnt<=0) return;
    if (free_str) {
        for (int i=0;i<cnt;i++) free(v->data[pos+i]);
    }
    memmove(v->data + pos, v->data + pos + cnt, (v->size - pos - cnt) * sizeof(char*));
    v->size -= cnt;
}
static void sv_free(StrVec *v, int free_str) {
    if (free_str) {
        for (int i=0;i<v->size;i++) free(v->data[i]);
    }
    free(v->data);
    v->data=NULL; v->size=0; v->cap=0;
}

static char* dup_str(const char* s) {
    size_t len = strlen(s);
    char* d = (char*)malloc(len+1);
    memcpy(d, s, len+1);
    return d;
}

static char* substr_dup(const char* s, int start) {
    size_t len = strlen(s);
    if ((size_t)start > len) start = (int)len;
    const char* p = s + start;
    return dup_str(p);
}

static void rstrip_newline(char* s) {
    size_t len = strlen(s);
    if (len && s[len-1]=='\n') s[len-1]='\0';
}

typedef struct {
    long long NN, MM, mm;
    StrVec frag_old; // extracted from '-' and ' '
    StrVec frag_new; // extracted from '+' and ' '
} Block;

typedef struct {
    Block *data;
    int size;
    int cap;
} BlockVec;

static void bv_init(BlockVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void bv_reserve(BlockVec *v, int cap){
    if (cap>v->cap){
        int ncap = v->cap? v->cap:8;
        while (ncap<cap) ncap<<=1;
        v->data=(Block*)realloc(v->data, ncap*sizeof(Block));
        v->cap=ncap;
    }
}
static Block* bv_emplace(BlockVec *v){
    bv_reserve(v, v->size+1);
    Block* b = &v->data[v->size++];
    b->NN=b->MM=b->mm=0;
    sv_init(&b->frag_old);
    sv_init(&b->frag_new);
    return b;
}
static void bv_free(BlockVec *v){
    for (int i=0;i<v->size;i++){
        sv_free(&v->data[i].frag_old, 1);
        sv_free(&v->data[i].frag_new, 1);
    }
    free(v->data);
    v->data=NULL; v->size=0; v->cap=0;
}

static int parse_posint(const char* s, int *i, long long *out){
    // first digit 1..9, then 0..9*
    int p = *i;
    if (!(s[p]>='1' && s[p]<='9')) return 0;
    long long val = s[p]-'0';
    p++;
    while (isdigit((unsigned char)s[p])) {
        val = val*10 + (s[p]-'0');
        p++;
    }
    *i = p;
    *out = val;
    return 1;
}

static int parse_header_line(const char* line, long long *NN, long long *MM, long long *nn_ignore, long long *mm){
    // exact format: "@@ -NN,MM +nn,mm @@"
    int i=0;
    if (!(line[i]=='@' && line[i+1]=='@')) return 0;
    i+=2;
    if (line[i++]!=' ') return 0;
    if (line[i++]!='-') return 0;
    long long tNN, tMM, tnn, tmm;
    if (!parse_posint(line, &i, &tNN)) return 0;
    if (line[i++]!=',') return 0;
    if (!parse_posint(line, &i, &tMM)) return 0;
    if (line[i++]!=' ') return 0;
    if (line[i++]!='+') return 0;
    if (!parse_posint(line, &i, &tnn)) return 0;
    if (line[i++]!=',') return 0;
    if (!parse_posint(line, &i, &tmm)) return 0;
    if (line[i++]!=' ') return 0;
    if (!(line[i]=='@' && line[i+1]=='@')) return 0;
    i+=2;
    if (line[i] != '\0') return 0;
    *NN = tNN; *MM = tMM; *nn_ignore = tnn; *mm = tmm;
    return 1;
}

static int str_equal(const char* a, const char* b){
    return strcmp(a,b)==0;
}

int main(void){
    // 读取 n
    long long n;
    if (scanf("%lld", &n)!=1) { printf("Patch is damaged.\n"); return 0; }
    int c = getchar(); // consume newline after n if present
    // 读原文件 n 行
    StrVec orig; sv_init(&orig);
    for (long long i=0;i<n;i++){
        char* line = NULL; size_t cap=0; ssize_t len = getline(&line, &cap, stdin);
        if (len<0){ // 不足 n 行
            free(line);
            sv_free(&orig, 1);
            printf("Patch is damaged.\n");
            return 0;
        }
        rstrip_newline(line);
        sv_push(&orig, line);
    }
    // 读取剩余补丁行（可能为0行）
    StrVec patch_raw; sv_init(&patch_raw);
    while (1){
        char* line=NULL; size_t cap=0; ssize_t len = getline(&line, &cap, stdin);
        if (len<0){ free(line); break; }
        rstrip_newline(line);
        sv_push(&patch_raw, line);
    }

    // 过滤注释行（# 开头的整行）
    StrVec patch; sv_init(&patch);
    for (int i=0;i<patch_raw.size;i++){
        const char* s = patch_raw.data[i];
        if (s[0]=='#') { free(patch_raw.data[i]); continue; }
        sv_push(&patch, patch_raw.data[i]); // 转移所有权
    }
    free(patch_raw.data); patch_raw.data=NULL; patch_raw.size=patch_raw.cap=0;

    // 找到以 @ 开头的行作为块起始
    // 记录块头在补丁中的行号
     int *heads = NULL; int hsz=0, hcap=0;
     #define HEAD_PUSH(x) do{ if(hsz==hcap){ hcap = hcap? hcap<<1:8; heads=(int*)realloc(heads, hcap*sizeof(int)); } heads[hsz++]=(x);}while(0)

    for (int i=0;i<patch.size;i++){
        if (patch.data[i][0]=='@') HEAD_PUSH(i);
    }
    if (hsz==0){ // 没有任何块
        sv_free(&orig, 1);
        sv_free(&patch, 1);
        free(heads);
        printf("Patch is damaged.\n");
        return 0;
    }

    // 解析每个块
    BlockVec blocks; bv_init(&blocks);
    for (int k=0;k<hsz;k++){
        int start = heads[k];
        int end = (k+1<hsz)? heads[k+1]-1 : patch.size-1;
        // 头行
        const char* header = patch.data[start];
        long long NN, MM, nn_ig, mm;
        if (!parse_header_line(header, &NN, &MM, &nn_ig, &mm)){
            // 格式不正确
            bv_free(&blocks);
            sv_free(&orig, 1);
            sv_free(&patch, 1);
            free(heads);
            printf("Patch is damaged.\n");
            return 0;
        }
        Block* b = bv_emplace(&blocks);
        b->NN = NN; b->MM = MM; b->mm = mm;
        // 体行
        for (int i=start+1; i<=end; i++){
            const char* ln = patch.data[i];
            if (!(ln[0]=='-' || ln[0]=='+' || ln[0]==' ')){
                bv_free(&blocks);
                sv_free(&orig, 1);
                sv_free(&patch, 1);
                free(heads);
                printf("Patch is damaged.\n");
                return 0;
            }
            char *content = substr_dup(ln, 1);
            if (ln[0]=='-' || ln[0]==' '){
                sv_push(&b->frag_old, dup_str(content));
            }
            if (ln[0]=='+'
                || ln[0]==' '){
                sv_push(&b->frag_new, content);
            } else {
                // ln[0]=='-' 已经把 content 复制进 frag_old，再释放本副本
                free(content);
            }
        }
        if ((long long)b->frag_old.size != b->MM || (long long)b->frag_new.size != b->mm){
            bv_free(&blocks);
            sv_free(&orig, 1);
            sv_free(&patch, 1);
            free(heads);
            printf("Patch is damaged.\n");
            return 0;
        }
        // 相邻块头部 NN 单调非重叠检查（预检查）
        if (k>0){
            Block* prev = &blocks.data[k-1];
            if (b->NN < prev->NN + prev->MM){
                bv_free(&blocks);
                sv_free(&orig, 1);
                sv_free(&patch, 1);
                free(heads);
                printf("Patch is damaged.\n");
                return 0;
            }
        }
    }

    // 所有块通过格式与计数检查后，应用补丁
    StrVec cur; sv_init(&cur);
    // 初始为原文件
    for (int i=0;i<orig.size;i++) sv_push(&cur, dup_str(orig.data[i]));

    long long accum_delta = 0;           // 累加前面块的 δ，影响后续块 NN 的基准
    long long shift_so_far = 0;          // 已应用到当前文件的行数净变化之和 (sum(mm - MM))
    long long prev_end_original = 0;     // 上一块在“原文件”坐标中的结束行（1-based），用于不重叠约束

    for (int k=0;k<blocks.size;k++){
        Block* b = &blocks.data[k];
        long long NN_eff = b->NN + accum_delta; // 当前块用于匹配的位置基准（已加上前面 δ）
        long long MM = b->MM, mm = b->mm;

        // 在 |δ| < MM 内搜索匹配，优先 |δ| 最小，若并列取 δ 最小（负数优先）
        int found = 0;
        long long best_delta = 0;
        long long s_match = 0;
        for (long long d=0; d<MM && !found; d++){
            long long cand[2];
            int cnt = 0;
            if (d==0){ cand[cnt++]=0; }
            else { cand[cnt++]= -d; cand[cnt++]= +d; }
            for (int t=0; t<cnt && !found; t++){
                long long delta = cand[t];
                long long s = NN_eff + delta; // 1-based 起点（原文件坐标）
                if (s < 1) continue;
                if (s + MM - 1 > (long long)orig.size) continue;
                // 不重叠条件（仅非首块）：s >= 上一块在原文中的结束后一行
                if (k > 0 && s < prev_end_original) continue;

                // 比较片段
                int ok = 1;
                for (long long i=0;i<MM;i++){
                    const char* a = orig.data[(int)(s-1+i)];
                    const char* bline = b->frag_old.data[(int)i];
                    if (!str_equal(a, bline)){ ok=0; break; }
                }
                if (ok){
                    found = 1;
                    best_delta = delta;
                    s_match = s;
                }
            }
        }
        if (!found){
            bv_free(&blocks);
            sv_free(&orig, 1);
            sv_free(&patch, 1);
            sv_free(&cur, 1);
            free(heads);
            printf("Patch is damaged.\n");
            return 0;
        }

        // 在当前文件中应用替换
        long long start_current_1based = s_match + shift_so_far; // 映射到当前文件坐标
        if (start_current_1based < 1 || start_current_1based - 1 + MM > (long long)cur.size){
            // 越界保护（理论不应发生）
            bv_free(&blocks);
            sv_free(&orig, 1);
            sv_free(&patch, 1);
            sv_free(&cur, 1);
            free(heads);
            printf("Patch is damaged.\n");
            return 0;
        }
        int pos0 = (int)(start_current_1based - 1);
        // 删除 MM 行
        sv_erase_range(&cur, pos0, (int)MM, 1);
        // 插入 mm 行
        // 复制 frag_new 的字符串，避免后续释放冲突
        char **ins = (char**)malloc((size_t)mm * sizeof(char*));
        for (int i=0;i<mm;i++) ins[i] = dup_str(b->frag_new.data[i]);
        sv_insert_range(&cur, pos0, ins, (int)mm);
        free(ins);

        // 更新状态
        accum_delta += best_delta;
        shift_so_far += (mm - MM);
        prev_end_original = s_match + MM;
        // 将该块与后续块的 NN 加上 δ：通过 accum_delta 体现，无需显式修改后续块
    }

    // 输出结果
    for (int i=0;i<cur.size;i++){
        puts(cur.data[i]);
    }

    // 清理
    bv_free(&blocks);
    sv_free(&orig, 1);
    sv_free(&patch, 1);
    sv_free(&cur, 1);
    free(heads);
    return 0;
}