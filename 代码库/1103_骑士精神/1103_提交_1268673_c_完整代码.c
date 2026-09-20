#include <stdio.h>
#include <string.h>

#define MAXD 15

// 目标棋盘 (假设题意图片对应)
// 11111
// 01111
// 00*11
// 00001
// 00000
static const char goal[] = "111110111100*110000100000";

int moveTo[25][8];
int moveCnt[25];

int best;
char start[26];

int h(const char *s){
    int diff = 0;
    for(int i=0;i<25;i++)
        if(s[i]!=goal[i]) diff++;
    return (diff + 1) >> 1; // 保持可采纳
}

// 迭代加深 DFS，增加避免回退与走法排序
static inline void sortMoves(int emptyPos, char *state, int moves[], int cnt, int scores[]){
    for(int i=0;i<cnt;i++){
        int from = moves[i];
        char backup = state[from];
        state[emptyPos] = backup;
        state[from] = '*';
        scores[i] = h(state);
        state[from] = backup;
        state[emptyPos] = '*';
    }
    // 插入排序按估值升序
    for(int i=1;i<cnt;i++){
        int mv = moves[i], sc = scores[i], j=i-1;
        while(j>=0 && scores[j] > sc){
            moves[j+1]=moves[j]; scores[j+1]=scores[j]; j--;
        }
        moves[j+1]=mv; scores[j+1]=sc;
    }
}

int dfs(int depth, int limit, int emptyPos, int lastEmpty, char *state){
    int estimate = h(state);
    if(estimate==0){
        best = depth;
        return 1;
    }
    if(depth + estimate > limit) return 0;

    int cnt = moveCnt[emptyPos];
    int moves[8], scores[8];
    for(int i=0;i<cnt;i++) moves[i]=moveTo[emptyPos][i];
    sortMoves(emptyPos, state, moves, cnt, scores);

    for(int i=0;i<cnt;i++){
        int from = moves[i];
        if(from == lastEmpty) continue; // 避免直接回退
        char backup = state[from];
        state[emptyPos] = backup;
        state[from] = '*';
        if(dfs(depth+1, limit, from, emptyPos, state)) return 1;
        state[from] = backup;
        state[emptyPos] = '*';
    }
    return 0;
}

int solve(char *initial){
    if(strcmp(initial, goal)==0) return 0;
    int emptyPos = -1;
    for(int i=0;i<25;i++)
        if(initial[i]=='*'){ emptyPos=i; break; }
    char state[26];
    memcpy(state, initial, 26);
    int startLimit = h(state); // 从启发下界开始
    for(int limit = startLimit; limit <= MAXD; limit++){
        if(dfs(0, limit, emptyPos, -1, state)) return best;
    }
    return -1;
}

int main(){
    // 预处理所有格子：哪些骑士能跳到该格(即该格为空时可从这些格子搬来)
    int dirs[8][2]={{1,2},{2,1},{-1,2},{2,-1},{1,-2},{-2,1},{-1,-2},{-2,-1}};
    for(int r=0;r<5;r++){
        for(int c=0;c<5;c++){
            int idx = r*5+c;
            moveCnt[idx]=0;
            for(int k=0;k<8;k++){
                int rr = r + dirs[k][0];
                int cc = c + dirs[k][1];
                if(rr>=0 && rr<5 && cc>=0 && cc<5){
                    moveTo[idx][moveCnt[idx]++] = rr*5+cc;
                }
            }
        }
    }

    int T;
    if(scanf("%d",&T)!=1) return 0;
    while(T--){
        char line[32];
        int pos=0;
        for(int i=0;i<5;i++){
            scanf("%s", line);
            for(int j=0;j<5;j++) start[pos++]=line[j];
        }
        start[25]='\0';
        int ans = solve(start);
        printf("%d\n", ans);
    }
    return 0;
}