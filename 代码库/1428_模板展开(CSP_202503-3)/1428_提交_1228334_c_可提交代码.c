#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MOD 1000000007LL
#define MAX_VARS 2048
#define MAX_NAME 64
#define LINE_BUF 100000

typedef long long ll;

typedef enum { OP_CONST = 0, OP_VAR = 1 } OpType;

typedef struct {
    OpType type;
    int varId;      // for OP_VAR
    int clen;       // for OP_CONST
} Operand;

typedef struct {
    int cnt;
    Operand *ops;
} Expr;

typedef struct {
    int type;   // 0: never assigned -> empty, 1: direct, 2: indirect
    ll lenMod;  // valid when type==1
    Expr *expr; // valid when type==2
} Var;

// --------- globals ----------
static Var vars[MAX_VARS];
static char varNames[MAX_VARS][MAX_NAME];
static int varCount = 0;

// memo for one evaluation pass
static ll memoLen[MAX_VARS];
static int memoMark[MAX_VARS];
static int curMark = 1;

// --------- utils ------------
static void chomp(char *s){
    int n = (int)strlen(s);
    while(n>0 && (s[n-1]=='\n' || s[n-1]=='\r')) s[--n] = '\0';
}

static int getVarId(const char *name){
    for(int i=0;i<varCount;i++){
        if(strcmp(varNames[i], name)==0) return i;
    }
    // new
    if(varCount >= MAX_VARS){
        fprintf(stderr, "Too many variables\n");
        exit(1);
    }
    strncpy(varNames[varCount], name, MAX_NAME-1);
    varNames[varCount][MAX_NAME-1]='\0';
    vars[varCount].type = 0;
    vars[varCount].lenMod = 0;
    vars[varCount].expr = NULL;
    return varCount++;
}

static void freeExpr(Expr *e){
    if(!e) return;
    free(e->ops);
    free(e);
}

static void assignDirect(int id, ll lenMod){
    if(vars[id].expr){
        freeExpr(vars[id].expr);
        vars[id].expr = NULL;
    }
    vars[id].type = 1;
    vars[id].lenMod = (lenMod % MOD + MOD) % MOD;
}

static void assignIndirect(int id, Expr *e){
    if(vars[id].expr){
        freeExpr(vars[id].expr);
    }
    vars[id].type = 2;
    vars[id].expr = e;
}

// forward
static ll evalVarLen(int id);

static ll evalExprLen(const Expr *e){
    if(e==NULL) return 0;
    ll s = 0;
    for(int i=0;i<e->cnt;i++){
        if(e->ops[i].type == OP_CONST){
            s += e->ops[i].clen;
        }else{
            s += evalVarLen(e->ops[i].varId);
        }
        if(s >= (ll)8*MOD) s %= MOD; // 控制中间值
    }
    return s % MOD;
}

static ll evalVarLen(int id){
    if(memoMark[id] == curMark) return memoLen[id];
    ll res = 0;
    if(vars[id].type == 1){
        res = vars[id].lenMod;
    }else if(vars[id].type == 2){
        res = evalExprLen(vars[id].expr);
    }else{
        res = 0;
    }
    memoMark[id] = curMark;
    memoLen[id] = res % MOD;
    return memoLen[id];
}

// 解析一行的表达式部分成为 Expr（用于间接赋值）
// tokens 从第三个单词开始（已用 strtok_r 切分），直到行末
static Expr* buildExprForIndirect(char *saveptr){
    Operand *ops = NULL;
    int cnt = 0, cap = 0;
    char *tok;
    while((tok = strtok_r(NULL, " \t\r\n", &saveptr)) != NULL){
        if(cnt == cap){
            cap = cap ? cap<<1 : 8;
            ops = (Operand*)realloc(ops, sizeof(Operand)*cap);
        }
        if(tok[0] == '$'){
            int id = getVarId(tok+1);
            ops[cnt].type = OP_VAR;
            ops[cnt].varId = id;
            ops[cnt].clen = 0;
        }else{
            ops[cnt].type = OP_CONST;
            ops[cnt].varId = -1;
            ops[cnt].clen = (int)strlen(tok);
        }
        cnt++;
    }
    Expr *e = (Expr*)malloc(sizeof(Expr));
    e->cnt = cnt;
    e->ops = ops ? ops : (Operand*)calloc(1,sizeof(Operand)); // 保证非空内存
    return e;
}

// 计算一行表达式的长度（用于直接赋值，按当前值求）
static ll evalExprLenNow(char *saveptr){
    ll s = 0;
    char *tok;
    while((tok = strtok_r(NULL, " \t\r\n", &saveptr)) != NULL){
        if(tok[0] == '$'){
            int id = getVarId(tok+1);
            s += evalVarLen(id);
        }else{
            s += (int)strlen(tok);
        }
        if(s >= (ll)8*MOD) s %= MOD;
    }
    return s % MOD;
}

int main(void){
    char line[LINE_BUF];
    if(!fgets(line, sizeof(line), stdin)) return 0;
    chomp(line);
    int n = 0;
    sscanf(line, "%d", &n);

    for(int i=0;i<n;i++){
        if(!fgets(line, sizeof(line), stdin)) break;
        chomp(line);
        if(line[0]=='\0') { i--; continue; }

        char *saveptr;
        char *tok = strtok_r(line, " \t\r\n", &saveptr);
        if(!tok) { continue; }

        if(strcmp(tok, "1")==0 || strcmp(tok, "2")==0){
            int op = (tok[0]=='1') ? 1 : 2;
            char *varName = strtok_r(NULL, " \t\r\n", &saveptr);
            if(!varName){ continue; }
            int id = getVarId(varName);

            if(op == 1){
                // 直接赋值：本次即时计算
                curMark++; // 新的一次求值 pass
                ll len = evalExprLenNow(saveptr);
                assignDirect(id, len);
            }else{
                // 间接赋值：保存表达式
                Expr *e = buildExprForIndirect(saveptr);
                assignIndirect(id, e);
            }
        }else if(strcmp(tok, "3")==0){
            char *varName = strtok_r(NULL, " \t\r\n", &saveptr);
            if(!varName){ puts("0"); continue; }
            int id = getVarId(varName);
            curMark++; // 新的一次求值 pass
            ll ans = evalVarLen(id) % MOD;
            printf("%lld\n", ans);
        }else{
            // 不符合语法，忽略
        }
    }

    // 释放可能残留的表达式内存
    for(int i=0;i<varCount;i++){
        if(vars[i].expr) freeExpr(vars[i].expr);
    }
    return 0;
}