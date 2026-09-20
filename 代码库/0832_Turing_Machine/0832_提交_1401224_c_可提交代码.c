#include<stdio.h>   // 引入标准输入输出库，用于 scanf/printf/puts 等
#include<stdlib.h>  // 引入通用工具库，用于 malloc/free/qsort 等
#include<string.h>  // 引入字符串库，用于 strcpy/strchr/strlen/memset 等

#define MAXP 1000    // 磁带最大长度（用作机头位置与磁带数组的上界）

/* 默认初始状态为 1，如题目初始为 0，请把该宏改为 0 */ // 说明：可通过宏配置初始状态编号
#ifndef START_STATE // 如果外部没有定义 START_STATE 宏
#define START_STATE 0   // 题面规定开始状态 q0=0，因此默认设为 0
#endif               // 结束条件编译

typedef struct{      // 定义一条转移规则的数据结构
    int s0;          // 上一步（当前）状态 s0
    char c0;         // 上一步机头所在格应读到的字符 c0（倒推时会写回去）
    int s1;          // 下一步状态 s1（正向执行后的状态）
    char c1;         // 下一步机头所在格的字符 c1（倒推时用于匹配当前带面）
    char x;          // 移动方向：'L'、'R' 或 '-'（不动）
} now;              // 结构体类型别名 now

static int nstate,nacc,nrej,m,nrules; // 全局：状态数、接受态数、拒绝态数、字母表大小、规则数
static int *acc=NULL,*rej=NULL;       // 全局：接受态数组、拒绝态数组（动态分配）
static now *rules=NULL;               // 全局：规则数组（动态分配）

/* 反向邻接表：按 s1 分组 */         // 说明：为倒推，从“下一步状态 s1”找到所有可能的前驱规则
static int *head=NULL,*nxt=NULL;      // head[v] 是到达状态 v 的规则链表头；nxt[i] 是规则 i 的下一条

/* 倒推时维护“当前（后缀步骤后的）磁带”，未知为'?' */ // 说明：curTape 表示我们正在倒推时的“已知/未知”带面
static char curTape[MAXP];            // 磁带内容，未知位置用 '?' 表示

/* 哪些格子已被确定（即使确定为 BLANK 也要保留到输出） */ // 说明：用于输出截断逻辑，避免丢掉被确定为“空白”的格
static unsigned char fixedPos[MAXP];  // 该位置是否已经被约束/确定（1=确定，0=未确定）

/* 时间戳访问标记，避免反复清零（state, headPos） */     // 说明：本来可用于剪枝，但当前剪枝代码被注释掉
static unsigned short seen[11005][MAXP]; // 访问标记表：seen[state][pos] = stamp 表示本轮访问过
static unsigned short stamp = 1;         // 当前轮次时间戳，从 1 开始递增

/* 空白符（根据输入字母表自适应） */ // 说明：题目可能用不同符号代表空白，程序根据字母表推断
static char BLANK = '#';                // 默认空白符为 '#'

/* 结果 */                             // 说明：找到可行初始配置后记录下来
static int ansHead=-1;                 // 输出用：初始机头位置
static char ansTape[MAXP+1];           // 输出用：初始磁带内容（以 '\0' 结尾）

/* 规范化破折号/回车为 ASCII '-'，放到文件作用域 */ // 说明：处理输入里可能出现的 Unicode 破折号
static void norm_dash(char *s){        // 将字符串中的特殊破折号替换为 ASCII '-'
    char buf[256]; int j=0;            // 临时缓冲区 buf，j 为写入位置
    for (int i=0; s[i] && j<255; ){    // i 遍历原串，保证 buf 不溢出并保留结尾 '\0'
        unsigned char c = (unsigned char)s[i]; // 取当前字节（无符号，便于比较高位字节）
        if(c=='\r'){ i++; continue; }  // 丢弃回车 CR，兼容 Windows 行尾
        // en/em-dash/minus → '-'     // 说明：把常见的 Unicode 短横/长横/减号统一成 '-'
        if(c==0xE2 && (unsigned char)s[i+1]==0x80 && // 判断 UTF-8 三字节序列前两字节
           ((unsigned char)s[i+2]==0x93 || (unsigned char)s[i+2]==0x94)){ // 0x93/0x94 对应 en/em dash
            buf[j++]='-';              // 写入标准 '-'
            i+=3;                      // 跳过这三个 UTF-8 字节
            continue;                  // 继续处理后续字符
        }
        if(c==0xE2 && (unsigned char)s[i+1]==0x88 && (unsigned char)s[i+2]==0x92){ // U+2212 负号
            buf[j++]='-';              // 写入标准 '-'
            i+=3;                      // 跳过 UTF-8 三字节
            continue;                  // 继续循环
        }
        buf[j++]=s[i++];               // 普通字符按原样拷贝到 buf
    }
    buf[j]=0;                          // 手动补上字符串结尾 '\0'
    strcpy(s,buf);                     // 把规范化后的内容写回原字符串
}

/* 二分查找小工具：判断 x 是否在升序数组 a 中（长度 n） */ // 说明：acc/rej 会先排序，然后用二分判断成员关系
static int in_set(const int *a,int n,int x){ // 返回 1 表示存在，0 表示不存在
    if(!a || n<=0) return 0;           // 空数组或长度非正，必然不存在
    int l=0,r=n-1;                     // 二分左右边界
    while(l<=r){                       // 标准二分循环条件
        int mid=(l+r)>>1;              // 取中点（右移等价除以 2）
        if(a[mid]==x) return 1;        // 命中则返回存在
        if(a[mid]<x) l=mid+1; else r=mid-1; // 根据比较结果缩小区间
    }
    return 0;                          // 循环结束仍未命中，返回不存在
}

/* 倒推：state1 为“下一步”的状态，p 为“下一步”的机头位置 */ // 说明：从某个“终局配置”往回找起始配置
static int dfs(int state1,int p){      // 尝试从 (state1, p, curTape) 倒推到 START_STATE，成功返回 1
    for(int ei=head[state1]; ei!=-1; ei=nxt[ei]){ // 枚举所有能到达 state1 的规则索引 ei
        now r = rules[ei];             // 取出该规则，便于后面使用

        // 由后到前计算上一步机头位置 prevP（处理左边界） // 说明：根据移动方向反推前一步机头在哪
        int cand[2], cnt=0;            // 候选 prevP 可能有 1 或 2 个（左边界时会分叉）
        if(r.x=='R'){                  // 正向往右走：prevP + 1 = p，因此 prevP = p - 1
            if(p-1>=0) cand[cnt++]=p-1; // 若不越界则加入候选
        }else if(r.x=='L'){            // 正向往左走：通常 prevP = p + 1，但 p=0 时有边界卡住的特殊情况
            if(p>0) {                  // 若 p>0，说明不是落在最左端
                cand[cnt++]=p+1;       // 候选为 p+1（反推）
            }else{                     // p==0：正向执行可能是从 1 左移到 0，也可能是在 0 左移仍停在 0
                // 允许两种前像：从 1 左移到 0，或在 0 左移被边界卡住 // 说明：题目边界模型允许“卡边界不动”
                if(1<MAXP) cand[cnt++]=1; // 候选一：prevP=1（从 1 左移到 0）
                cand[cnt++]=0;         // 候选二：prevP=0（从 0 左移仍为 0）
            }
        }else{                         // r.x 不是 L/R 时按不动处理
            cand[cnt++]=p;             // 不动：prevP = p
        }

        for(int k=0;k<cnt;k++){        // 枚举每个可能的 prevP
            int prevP = cand[k];       // 取出当前候选的前一步机头位置
            if(prevP<0 || prevP>=MAXP) continue; // 防御性：越界则跳过该候选

            // 当前应为 c1，回滚为 c0（符号不匹配直接剪枝） // 说明：倒推时当前位置必须能匹配“下一步写完后的符号”
            char old = curTape[prevP]; // 记录回溯前该格原来的字符
            unsigned char oldF = fixedPos[prevP]; // 记录回溯前该格是否已被确定
            if(old!='?' && old!=r.c1) continue; // 若该格已确定且不等于期望 c1，则该规则不可能成立
            curTape[prevP] = r.c0;     // 反向回滚：把该格从 c1 回滚到 c0（即“上一步读到/写前”的字符）
            fixedPos[prevP] = 1;       // 标记该位置已被确定（因为我们刚施加了约束）

            if(r.s0==START_STATE){     // 如果上一步状态已经是起始状态
                // 命中初始状态，形成一个可行初始配置 // 说明：此时 curTape 代表一种可行的初始磁带（带 '?'）
                ansHead = prevP;       // 初始机头位置就是 prevP
                for(int i=0;i<MAXP;i++) ansTape[i]=(curTape[i]=='?' ? BLANK : curTape[i]); // 将未知 '?' 填充为空白符
                // 右端截断到“最右非空白或最右被确定格” // 说明：输出不要无限长，只保留必要前缀
                int last = -1;         // last 记录需要输出的最后一个下标
                for(int i=0;i<MAXP;i++){ // 扫描整条磁带
                    if(ansTape[i]!=BLANK || fixedPos[i]) last = i; // 非空白或曾被确定的位置都应保留
                }
                ansTape[last+1] = '\0'; // 在 last+1 处加字符串结束符，完成截断
                return 1;              // 找到一个可行解，直接返回成功
            }else{                     // 还没到起始状态，需要继续倒推
                // 若上一步是拒绝状态，直接跳过 // 说明：要求初始到接受的路径中不落入拒绝
                if(in_set(rej,nrej,r.s0)){ // 判断前一步状态是否为拒绝态
                    curTape[prevP] = old;  // 回溯恢复该格字符
                    fixedPos[prevP] = oldF; // 回溯恢复确定标记
                    continue;          // 跳过该规则/候选，尝试下一种
                }

                // 去掉(state, headPos)剪枝，避免误剪 // 说明：剪枝可能在存在多种带面约束时误判可达性
                // if(seen[r.s0][prevP]!=stamp){ // 若本轮尚未访问过该 (state,pos)
                //     seen[r.s0][prevP]=stamp; // 标记为已访问
                    if(dfs(r.s0, prevP)) return 1; // 递归倒推，若成功则一路返回
                // }                  // 结束剪枝判断（当前注释掉）
            }
            // 回溯                         // 说明：撤销本次尝试对 curTape/fixedPos 的修改
            curTape[prevP] = old;         // 恢复该格原字符
            fixedPos[prevP] = oldF;       // 恢复该格原确定状态
        }
    }
    return 0;                          // 所有规则与候选都无法倒推到起始状态，返回失败
}

static int cmpInt(const void*a,const void*b){ return *(const int*)a-*(const int*)b; } // qsort 比较函数：升序排序整型

int main(void){                        // 程序入口
    if(scanf("%d",&nstate)!=1) return 0; // 读取状态总数，失败则直接退出

    // 接受集                          // 说明：读取接受态数量与列表
    if(scanf("%d",&nacc)==1 && nacc>0){  // 若成功读到 nacc 且 nacc>0
        acc = (int*)malloc(sizeof(int)*nacc); // 分配接受态数组
        for(int i=0;i<nacc;i++) scanf("%d",&acc[i]); // 逐个读入接受态编号
        qsort(acc,nacc,sizeof(int),cmpInt); // 排序，便于后续二分查找
    }

    // 拒绝集                          // 说明：读取拒绝态数量与列表
    if(scanf("%d",&nrej)==1 && nrej>0){  // 若成功读到 nrej 且 nrej>0
        rej = (int*)malloc(sizeof(int)*nrej); // 分配拒绝态数组
        for(int i=0;i<nrej;i++) scanf("%d",&rej[i]); // 逐个读入拒绝态编号
        qsort(rej,nrej,sizeof(int),cmpInt); // 排序，便于后续二分查找
    }

    // 磁带字母表（推断空白符）         // 说明：根据输入的字母表选择空白符
    if(scanf("%d",&m)!=1) return 0;    // 读取字母表大小 m，失败则退出
    char alphabet[256]={0};            // 保存字母表字符串（最多 255 字符）
    scanf("%255s",alphabet);           // 读入字母表内容
    if(strchr(alphabet,'#')) BLANK = '#'; // 若字母表包含 '#'，优先用它当空白
    else if(strchr(alphabet,'*')) BLANK = '*'; // 否则若包含 '*'，用 '*' 当空白
    else BLANK = alphabet[ (m>0 && m-1<(int)strlen(alphabet)) ? m-1 : 0 ]; // 否则用字母表末尾字符（带防御）

    // 规则                            // 说明：读取转移规则并存入数组
    if(scanf("%d",&nrules)!=1) return 0; // 读取规则数 nrules，失败则退出
    rules = (now*)malloc(sizeof(now)*nrules); // 分配规则数组

    // 读取 nrules 行                   // 说明：规则逐行读取，兼容含 Unicode 破折号的输入
    {
        int ch; while((ch=getchar())!='\n' && ch!=EOF){} // 吃掉上一行末尾残留的换行，准备 fgets 读整行
        char line[256];                                  // 每条规则行的缓冲区
        for(int i=0;i<nrules;i++){                       // 循环读取 nrules 行
            if(!fgets(line,sizeof(line),stdin)) return 0; // 读取一行失败则退出
            norm_dash(line);                             // 规范化破折号，确保 '-' 解析正确
            int s0,s1; char ch0[8],ch1[8],mv[8];        // 临时变量：状态与字符/移动字段
            if(sscanf(line," %d %1s %d %1s %1s",&s0,ch0,&s1,ch1,mv)!=5) return 0; // 按格式解析失败则退出
            rules[i].s0=s0; rules[i].c0=ch0[0];         // 保存前状态与读/写字符 c0
            rules[i].s1=s1; rules[i].c1=ch1[0];         // 保存后状态与字符 c1
            rules[i].x = (mv[0]=='L'||mv[0]=='R'||mv[0]=='-')? mv[0] : '-'; // 仅接受 L/R/-，否则按不动处理
            if(in_set(acc,nacc,rules[i].s1) || in_set(rej,nrej,rules[i].s1)) rules[i].x='-'; // 到达终止态则强制不移动
        }
    }

    // 反向邻接表                       // 说明：按 s1 建链，便于从某个 state1 找所有前驱规则
    head = (int*)malloc(sizeof(int)*nstate); // 为每个状态分配链表头数组
    nxt  = (int*)malloc(sizeof(int)*nrules); // 为每条规则分配 next 指针数组
    for(int i=0;i<nstate;i++) head[i]=-1;    // 初始化：所有链表为空（-1 表示无）

    // 关键：倒序插入，保证遍历顺序与输入顺序一致 // 说明：链表头插会反转顺序，倒序插入可恢复原顺序
    for(int i=nrules-1;i>=0;i--){            // 从最后一条规则往前插
        int v = rules[i].s1;                 // v 是该规则的“下一步状态”，作为分组键
        if(v<0 || v>=nstate) continue;       // 防御性：状态编号不合法则忽略该规则
        nxt[i]=head[v];                      // 规则 i 的 next 指向当前链表头
        head[v]=i;                           // 更新链表头为规则 i
    }

    int found = 0;                           // 标记是否已经找到可行初始配置
    for(int ai=0; ai<nacc && !found; ++ai){  // 依次尝试每个接受态作为倒推起点
        int accState = acc[ai];              // 当前尝试的接受态
        for(int pend=0; pend<MAXP && !found; ++pend){ // 枚举“结束时机头位置” pend
            // 初始化磁带                    // 说明：每次尝试都从全未知带面开始
            for(int i=0;i<MAXP;i++){ curTape[i]='?'; fixedPos[i]=0; } // 设置所有格未知且未确定
            // 新一轮时间戳（16 位循环，用尽时清空） // 说明：stamp 溢出后清零 seen，避免冲突
            stamp++;                         // 时间戳加一，代表新一轮搜索
            if(stamp==0){ memset(seen,0,sizeof(seen)); stamp=1; } // 若回绕到 0，则清表并从 1 重新开始
            if(dfs(accState, pend)) found=1; // 从 (接受态, 机头位置) 开始倒推，若成功则置 found
        }
    }

    if(!found){                              // 若遍历完仍未找到任何可行初始配置
        printf("0\n");                       // 按题意输出机头位置为 0（表示无解的约定输出）
        printf("%c\n", BLANK);               // 输出仅包含一个空白符的磁带
    }else{                                   // 若找到了可行初始配置
        printf("%d\n", ansHead);             // 输出初始机头位置
        puts(ansTape[0]? ansTape : (char[]){BLANK,0}); // 输出磁带；若为空串则输出单个空白符
    }

    free(acc); free(rej); free(rules); free(head); free(nxt); // 释放动态分配的内存，避免泄漏
    return 0;                               // 正常结束程序
}//Scorpio®