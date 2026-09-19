#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*
  液体试管拼图最短步数（BFS）
  - 试管容量固定 4
  - 输入每管自底向上 4 个数字，0 表示空
  - 状态编码：按管序，每管 4 字节（底到顶），合计 4*n 字节
  - 倾倒规则：若 y 顶为空或等于 x 顶色，且 y 未满，则将 x 顶部连续同色段尽可能倒入（受 y 剩余空间限制）
  - 目标：所有非空试管都为单色且满（4 层）
  - 输出任一最优方案（0-based 下标）
*/

typedef struct {
	int from_idx;
	int to_idx;
} Move;

typedef struct {
	int prev_index;   // 上一状态在队列中的索引，起点为 -1
	Move mv;          // 从 prev -> 当前 的操作
} Trace;

// 简单动态数组队列存储状态键（每个键为 4*n 字节）
typedef struct {
	uint8_t *data;    // 扁平化存储：size * key_len
	int size;
	int capacity;
	int key_len;
} KeyArray;

static void key_array_init(KeyArray *ka, int key_len) {
	ka->data = NULL;
	ka->size = 0;
	ka->capacity = 0;
	ka->key_len = key_len;
}

static int key_array_append(KeyArray *ka, const uint8_t *key) {
	if (ka->size == ka->capacity) {
		int new_cap = ka->capacity ? ka->capacity * 2 : 1024;
		size_t new_bytes = (size_t)new_cap * (size_t)ka->key_len;
		uint8_t *nd = (uint8_t*)realloc(ka->data, new_bytes);
		if (!nd) {
			fprintf(stderr, "Memory allocation failed\n");
			exit(1);
		}
		ka->data = nd;
		ka->capacity = new_cap;
	}
	memcpy(ka->data + (size_t)ka->size * (size_t)ka->key_len, key, (size_t)ka->key_len);
	return ka->size++;
}

static const uint8_t* key_array_get(const KeyArray *ka, int idx) {
	return ka->data + (size_t)idx * (size_t)ka->key_len;
}

// 追踪信息数组
static Trace *trace_arr = NULL;
static int trace_size = 0;
static int trace_capacity = 0;

static void trace_append(Trace t) {
	if (trace_size == trace_capacity) {
		int new_cap = trace_capacity ? trace_capacity * 2 : 1024;
		Trace *nt = (Trace*)realloc(trace_arr, (size_t)new_cap * sizeof(Trace));
		if (!nt) {
			fprintf(stderr, "Memory allocation failed\n");
			exit(1);
		}
		trace_arr = nt;
		trace_capacity = new_cap;
	}
	trace_arr[trace_size++] = t;
}

// FNV-1a 64-bit 哈希
static uint64_t fnv1a_hash(const uint8_t *data, int len) {
	uint64_t h = 1469598103934665603ULL;
	for (int i = 0; i < len; ++i) {
		h ^= (uint64_t)data[i];
		h *= 1099511628211ULL;
	}
	return h;
}

// 开放定址 hash -> index+1（0 表空）
typedef struct {
	int *slots;       // 存储为 (idx+1)，0 表空
	int capacity;     // 必须是 2 的幂
	int used;         // 装填数
	int key_len;
	const KeyArray *keys; // 指向队列的键数组以便比较
} HashSet;

static int next_power_of_two(int x) {
	int p = 1;
	while (p < x) p <<= 1;
	return p;
}

static void hashset_init(HashSet *hs, int expected_size, int key_len, const KeyArray *keys) {
	int cap = next_power_of_two(expected_size * 2);
	if (cap < 2048) cap = 2048;
	hs->capacity = cap;
	hs->slots = (int*)calloc((size_t)cap, sizeof(int));
	if (!hs->slots) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}
	hs->used = 0;
	hs->key_len = key_len;
	hs->keys = keys;
}

static void hashset_free(HashSet *hs) {
	free(hs->slots);
	hs->slots = NULL;
	hs->capacity = 0;
	hs->used = 0;
	hs->key_len = 0;
	hs->keys = NULL;
}

// 查找是否存在，若不存在可返回插入位置
static bool hashset_contains(HashSet *hs, const uint8_t *key, int *slot_out) {
	uint64_t h = fnv1a_hash(key, hs->key_len);
	int mask = hs->capacity - 1;
	int pos = (int)(h & (uint64_t)mask);
	for (;;) {
		int v = hs->slots[pos];
		if (v == 0) {
			if (slot_out) *slot_out = pos;
			return false;
		}
		const uint8_t *exist = key_array_get(hs->keys, v - 1);
		if (memcmp(exist, key, (size_t)hs->key_len) == 0) {
			if (slot_out) *slot_out = pos;
			return true;
		}
		pos = (pos + 1) & mask;
	}
}

static void hashset_insert_at(HashSet *hs, int slot_pos, int index_plus_one) {
	// 线性探测插入
	int mask = hs->capacity - 1;
	int pos = slot_pos;
	for (;;) {
		if (hs->slots[pos] == 0) {
			hs->slots[pos] = index_plus_one;
			hs->used++;
			return;
		}
		pos = (pos + 1) & mask;
	}
}

// Tube 工具函数
static inline int tube_size(const uint8_t *state, int n, int tube_idx) {
	const uint8_t *base = state + tube_idx * 4;
	int sz = 0;
	for (int i = 0; i < 4; ++i) {
		if (base[i] != 0) sz++;
	}
	return sz;
}

static inline uint8_t tube_top_color(const uint8_t *state, int n, int tube_idx) {
	int sz = tube_size(state, n, tube_idx);
	if (sz == 0) return 0;
	return state[tube_idx * 4 + (sz - 1)];
}

static int tube_top_run(const uint8_t *state, int n, int tube_idx) {
	int sz = tube_size(state, n, tube_idx);
	if (sz == 0) return 0;
	const uint8_t *base = state + tube_idx * 4;
	uint8_t c = base[sz - 1];
	int run = 1;
	for (int i = sz - 2; i >= 0; --i) {
		if (base[i] == c) run++;
		else break;
	}
	return run;
}

static inline int tube_space(const uint8_t *state, int n, int tube_idx) {
	return 4 - tube_size(state, n, tube_idx);
}

static bool is_goal(const uint8_t *state, int n) {
	for (int t = 0; t < n; ++t) {
		const uint8_t *base = state + t * 4;
		// 统计非零层数
		int sz = 0;
		for (int i = 0; i < 4; ++i) if (base[i] != 0) sz++;
		if (sz == 0) continue;
		// 目标要求：满且同色
		if (sz != 4) return false;
		uint8_t c = base[0];
		if (c == 0) return false;
		for (int i = 1; i < 4; ++i) {
			if (base[i] != c) return false;
		}
	}
	return true;
}

// 生成从 x->y 的新状态（若合法返回 true 并写入 out）
static bool try_pour(const uint8_t *state, int n, int x, int y, uint8_t *out) {
	if (x == y) return false;
	int sx = tube_size(state, n, x);
	if (sx == 0) return false;
	int sy = tube_size(state, n, y);
	if (sy == 4) return false;

	uint8_t cx = tube_top_color(state, n, x);
	uint8_t cy = tube_top_color(state, n, y);
	if (sy != 0 && cy != cx) return false;

	// 拷贝原状态
	memcpy(out, state, (size_t)n * 4);

	// 可以倒的量：顶部同色连续段 vs y 剩余空间
	int run = tube_top_run(state, n, x);
	int space = 4 - sy;
	int amount = run < space ? run : space;
	if (amount <= 0) return false;

	// 执行倒液体：从 x 弹出 amount 次，压入 y
	int new_sx = sx - amount;
	int new_sy = sy + amount;

	// 写回 x（保留底部 new_sx 个）
	// x 区间：[0..new_sx-1] 保留，其余清 0
	uint8_t *bx = out + x * 4;
	// 原底部保留不变，清空多余
	for (int i = new_sx; i < 4; ++i) bx[i] = 0;

	// 写入 y 顶部
	uint8_t *by = out + y * 4;
	for (int i = sy; i < new_sy; ++i) {
		by[i] = cx;
	}
	return true;
}

int main(void) {
	int n;
	if (scanf("%d", &n) != 1) return 0;
	if (n <= 0 || n > 10) return 0;

	// 读取输入，自底向上 4 个数，0 表空
	// 规范化为每管 4 字节，自底向上紧凑存放非零
	uint8_t *start = (uint8_t*)calloc((size_t)n * 4, 1);
	if (!start) {
		fprintf(stderr, "Memory allocation failed\n");
		return 1;
	}
	int total_liquid = 0;
	for (int t = 0; t < n; ++t) {
		int a[4];
		for (int i = 0; i < 4; ++i) {
			if (scanf("%d", &a[i]) != 1) a[i] = 0;
		}
		// 紧凑收集非零（保持底到顶顺序）
		int sz = 0;
		for (int i = 0; i < 4; ++i) {
			if (a[i] != 0) {
				start[t * 4 + sz] = (uint8_t)a[i];
				sz++;
				total_liquid++;
			}
		}
	}
	// 题面保证总份数不超过 20
	(void)total_liquid;

	const int key_len = n * 4;

	// 初始化结构
	KeyArray keys;
	key_array_init(&keys, key_len);

	// BFS 队列用索引 [head..tail)
	int *queue = NULL;
	int q_cap = 0, q_head = 0, q_tail = 0;

	// 放入起点
	int start_idx = key_array_append(&keys, start);
	free(start);

	Trace t0;
	t0.prev_index = -1;
	t0.mv.from_idx = -1;
	t0.mv.to_idx = -1;
	trace_append(t0);

	// 预估容量：状态数一般可控，设 1<<20 哈希容量上限足够
	HashSet visited;
	hashset_init(&visited, 1 << 18, key_len, &keys); // 约 262k 期望，容量 2^19 起
	int slot_pos = 0;
	if (!hashset_contains(&visited, key_array_get(&keys, start_idx), &slot_pos)) {
		hashset_insert_at(&visited, slot_pos, start_idx + 1);
	}

	// 队列 push
	q_cap = 1024;
	queue = (int*)malloc((size_t)q_cap * sizeof(int));
	if (!queue) {
		fprintf(stderr, "Memory allocation failed\n");
		return 1;
	}
	queue[q_tail++] = start_idx;

	int goal_idx = -1;
	uint8_t *buffer = (uint8_t*)malloc((size_t)key_len);
	if (!buffer) {
		fprintf(stderr, "Memory allocation failed\n");
		return 1;
	}

	while (q_head < q_tail) {
		int cur = queue[q_head++];
		const uint8_t *state = key_array_get(&keys, cur);

		// 检查目标
		if (is_goal(state, n)) {
			goal_idx = cur;
			break;
		}

		// 枚举所有可能倒法
		for (int x = 0; x < n; ++x) {
			if (tube_size(state, n, x) == 0) continue;
			for (int y = 0; y < n; ++y) {
				if (x == y) continue;
				if (!try_pour(state, n, x, y, buffer)) continue;

				// 去重
				int pos = 0;
				if (hashset_contains(&visited, buffer, &pos)) continue;

				// 记录新状态
				int idx = key_array_append(&keys, buffer);
				Trace tr;
				tr.prev_index = cur;
				tr.mv.from_idx = x;
				tr.mv.to_idx = y;
				trace_append(tr);

				// 标记 visited
				hashset_insert_at(&visited, pos, idx + 1);

				// 入队
				if (q_tail == q_cap) {
					int new_cap = q_cap * 2;
					int *nq = (int*)realloc(queue, (size_t)new_cap * sizeof(int));
					if (!nq) {
						fprintf(stderr, "Memory allocation failed\n");
						return 1;
					}
					queue = nq;
					q_cap = new_cap;
				}
				queue[q_tail++] = idx;
			}
		}
	}

	free(buffer);
	hashset_free(&visited);
	free(queue);

	if (goal_idx == -1) {
		// 理论上应总可达；若不可达，按题意输出 0 步
		printf("0\n");
		return 0;
	}

	// 回溯路径
	int path_cap = 128, path_len = 0;
	Move *path = (Move*)malloc((size_t)path_cap * sizeof(Move));
	if (!path) {
		fprintf(stderr, "Memory allocation failed\n");
		return 1;
	}
	int cur = goal_idx;
	while (trace_arr[cur].prev_index != -1) {
		if (path_len == path_cap) {
			path_cap *= 2;
			Move *np = (Move*)realloc(path, (size_t)path_cap * sizeof(Move));
			if (!np) {
				fprintf(stderr, "Memory allocation failed\n");
				return 1;
			}
			path = np;
		}
		path[path_len++] = trace_arr[cur].mv;
		cur = trace_arr[cur].prev_index;
	}

	// 输出
	printf("%d\n", path_len);
	for (int i = path_len - 1; i >= 0; --i) {
		printf("%d %d\n", path[i].from_idx, path[i].to_idx);
	}

	free(path);
	free(trace_arr);
	free(keys.data);
	return 0;
}