#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const long long INF = 1e18;
const int MAXN = 105;
const int MAXC = 10005; 

int n, m;
long long C_cap;
int T_queries;

struct NodeInfo {
    int p;
    long long c; // changed to long long
} nodes[MAXN];

struct Edge {
    int v;
    int l;
};

// Use Adjacency List for faster neighbor iteration
struct AdjNode {
    int to;
    long long len;
};
vector<AdjNode> graph[MAXN];

// Raw edges for doubling
long long doubling[18][MAXN][MAXN]; // 2^17 > 100000

// Macro edges: u -> v, max distance
// Optimized to only store valid neighbors
struct MacroEdge {
    int v;
    long long dist;
};
vector<MacroEdge> macro_adj[MAXN];

// Max distance achievable after refueling at u and then stopping
long long final_max[MAXN];

// DP table
long long best_dist[MAXC][MAXN]; // [budget][start_node]
long long monotonic_best[MAXN][MAXC]; 

// Step doubling for "Max dist in <= 2^k steps" (For DAGs and short paths)
// Actually we can just perform a linear scan 1..N to cover short paths.
// No need for complex DoublingMax unless we suspect short paths > N?
// If limit is huge and no cycles, we just won't find anything in the deep steps.
// So scanning 1..N and limit-K..limit is optimal.

void perform_scan(int u, long long start_step, int window_len, long long limit) {
    // If start_step > limit, return
    if (start_step > limit) return;
    
    // Safety clamp
    long long end_step = min(limit, start_step + window_len);
    
    static long long current_dists[MAXN];
    for(int i=1; i<=n; ++i) current_dists[i] = -1;
    
    if (start_step == 0) {
        current_dists[u] = 0;
    } else {
        // Binary lifting to get to start_step
        bool first = true;
        for (int b = 0; b < 18; ++b) {
            if ((start_step >> b) & 1) {
                static long long next_dists[MAXN];
                for(int i=1; i<=n; ++i) next_dists[i] = -1;

                if (first) {
                    for(int v=1; v<=n; ++v) {
                        next_dists[v] = doubling[b][u][v];
                    }
                    first = false;
                } else {
                    for (int mid = 1; mid <= n; ++mid) {
                        if (current_dists[mid] == -1) continue;
                        for (int v = 1; v <= n; ++v) {
                            if (doubling[b][mid][v] != -1) {
                                next_dists[v] = max(next_dists[v], current_dists[mid] + doubling[b][mid][v]);
                            }
                        }
                    }
                }
                for(int i=1; i<=n; ++i) current_dists[i] = next_dists[i];
            }
        }
    }
    
    // Check if current_dists is all -1 (skipped over short path)
    bool any_reachable = false;
    for(int i=1; i<=n; ++i) if(current_dists[i]!=-1) any_reachable=true;
    if (!any_reachable && start_step > 0) return;

    for (long long s = start_step; s <= end_step; ++s) {
        long long best_any = -1;
        for (int v = 1; v <= n; ++v) best_any = max(best_any, current_dists[v]);
        if (best_any != -1) final_max[u] = max(final_max[u], best_any);

        if (s > 0) {
            for (int v = 1; v <= n; ++v) {
                if (current_dists[v] != -1) {
                    if (s > limit - nodes[v].c) {
                        // Found a macro edge
                        // Store in temporary map or just check if duplicate?
                        // We will add to vector later. For now just update a matrix?
                        // Or straight to vector if we clear it.
                        // Since we might visit same v multiple times with increasing distance,
                        // we should maintain a BEST array then flush to vector.
                        // Can't access best_dist directly.
                        // Let's assume we fill a global matrix first then sparsify.
                        // Or just use matrix here since N is small.
                    }
                }
            }
        }

        if (s == end_step) break;

        static long long next_dists[MAXN];
        for(int i=1; i<=n; ++i) next_dists[i] = -1;
        
        for (int curr = 1; curr <= n; ++curr) {
            if (current_dists[curr] == -1) continue;
            for (const auto& edge : graph[curr]) {
                next_dists[edge.to] = max(next_dists[edge.to], current_dists[curr] + edge.len);
            }
        }
        for(int i=1; i<=n; ++i) current_dists[i] = next_dists[i];
    }
    
    // For saving results from this scan, we can assume caller handles it or we use a global matrix
}

long long macro_matrix[MAXN][MAXN];

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    if (!(cin >> n >> m >> C_cap >> T_queries)) return 0;

    for (int i = 1; i <= n; ++i) {
        cin >> nodes[i].p >> nodes[i].c;
    }

    for (int i = 0; i < m; ++i) {
        int u, v, l;
        cin >> u >> v >> l;
        // Keep best edge u->v
        // Wait, multiple edges?
        // Graph is multigraph?
        // "from one ... possible multiple roads".
        // Adj should keep max.
        // For adjacency list, we can keep all parallel edges or just max.
        // Max is sufficient for "Longest Path".
        
        bool found = false;
        for(auto& edge : graph[u]) {
            if(edge.to == v) {
                edge.len = max(edge.len, (long long)l);
                found = true;
                break;
            }
        }
        if(!found) graph[u].push_back({v, (long long)l});
    }

    // Initialize doubling[0]
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            doubling[0][i][j] = -1;
        }
        for(const auto& edge : graph[i]) {
            doubling[0][i][edge.to] = max(doubling[0][i][edge.to], edge.len);
        }
    }
    
    
    // Precompute doubling
    for (int k = 1; k < 18; ++k) {
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                doubling[k][i][j] = -1;
            }
        }
        
        for (int mid = 1; mid <= n; ++mid) {
             for (int i = 1; i <= n; ++i) {
                if (doubling[k-1][i][mid] == -1) continue;
                long long val1 = doubling[k-1][i][mid];
                for (int j = 1; j <= n; ++j) {
                    if (doubling[k-1][mid][j] != -1) {
                         doubling[k][i][j] = max(doubling[k][i][j], val1 + doubling[k-1][mid][j]);
                    }
                }
            }
        }
    }

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) macro_matrix[i][j] = -1;
        final_max[i] = 0;
    }

    // Compute Macro
    // Small window size for short paths (DAG cases)
    // Large window shift for cycles
    
    for (int u = 1; u <= n; ++u) {
        long long limit = min((long long)nodes[u].c, C_cap);
        if (limit == 0) continue;
        
        // Use a matrix to accumulate results for this u over potentially two scans
        static long long u_macro[MAXN];
        for(int v=1; v<=n; ++v) u_macro[v] = -1;
        long long u_final = 0;

        auto run_scan = [&](long long start_s, int steps) {
            if (start_s > limit) return;
            long long end_s = min(limit, start_s + steps);
            
            static long long cur[MAXN];
            for(int i=1; i<=n; ++i) cur[i] = -1;
            
            if (start_s == 0) cur[u] = 0;
            else {
                 bool first = true;
                for (int b = 0; b < 18; ++b) {
                    if ((start_s >> b) & 1) {
                        static long long nxt[MAXN];
                        for(int i=1; i<=n; ++i) nxt[i] = -1;
                        if (first) {
                            for(int v=1; v<=n; ++v) nxt[v] = doubling[b][u][v];
                            first = false;
                        } else {
                            for (int mid = 1; mid <= n; ++mid) {
                                if (cur[mid] == -1) continue;
                                for (int v = 1; v <= n; ++v) {
                                    if (doubling[b][mid][v] != -1) {
                                        nxt[v] = max(nxt[v], cur[mid] + doubling[b][mid][v]);
                                    }
                                }
                            }
                        }
                        for(int i=1; i<=n; ++i) cur[i] = nxt[i];
                    }
                }
            }
            
            // Check reachability
            bool ok = false; 
            for(int i=1; i<=n; ++i) if(cur[i] != -1) ok = true;
            if (!ok && start_s > 0) return; // Cannot reach this step count
            
            for (long long s = start_s; s <= end_s; ++s) {
                // Update final_max
                long long best_here = -1;
                for(int v=1; v<=n; ++v) best_here = max(best_here, cur[v]);
                if (best_here != -1) u_final = max(u_final, best_here);

                if (s > 0) {
                     for(int v=1; v<=n; ++v) {
                         if (cur[v] != -1) {
                             if (s > limit - nodes[v].c) { // condition: rem fuel < c[v]
                                 u_macro[v] = max(u_macro[v], cur[v]);
                             }
                         }
                     }
                }
                
                if (s == end_s) break;
                
                static long long nxt[MAXN];
                for(int i=1; i<=n; ++i) nxt[i] = -1;
                for(int curr=1; curr<=n; ++curr) {
                    if (cur[curr] == -1) continue;
                     for(int k=0; k<graph[curr].size(); ++k) {
                         int v_to = graph[curr][k].to;
                         long long len = graph[curr][k].len;
                         nxt[v_to] = max(nxt[v_to], cur[curr] + len);
                     }
                }
                for(int i=1; i<=n; ++i) cur[i] = nxt[i];
            }
        };
        
        // 1. Scan 0..N+5
        run_scan(0, n + 5);
        
        // 2. Scan Limit-N..Limit (if distinct enough)
        // If limit is small, first scan covered it.
        // Overlap is fine (idempotent max).
        if (limit > n + 5) {
             long long start2 = max(0LL, limit - (n + 5));
             // Ensure we don't start before 0
             run_scan(start2, n + 5 + 5); // scan a bit
        }
        
        final_max[u] = u_final;
        for(int v=1; v<=n; ++v) {
            if (u_macro[v] != -1) {
                macro_adj[u].push_back({v, u_macro[v]});
            }
        }
    }
    
    // DP
    for (int b = 0; b <= 10000; ++b) {
        for (int i = 1; i <= n; ++i) best_dist[b][i] = 0;
    }

    for (int b = 0; b <= 10000; ++b) {
        for (int u = 1; u <= n; ++u) {
            if (b >= nodes[u].p) {
                int rem_b = b - nodes[u].p;
                long long current_res = best_dist[b][u];
                
                current_res = max(current_res, final_max[u]);
                
                // Using sparser macro graph
                for (const auto& edge : macro_adj[u]) {
                     current_res = max(current_res, edge.dist + best_dist[rem_b][edge.v]);
                }
                best_dist[b][u] = current_res;
            }
        }
    }
    
    for (int u = 1; u <= n; ++u) {
        monotonic_best[u][0] = best_dist[0][u];
        for (int b = 1; b <= 10000; ++b) {
            monotonic_best[u][b] = max(monotonic_best[u][b-1], best_dist[b][u]);
        }
    }

    for (int k = 0; k < T_queries; ++k) {
        int s, q;
        long long d;
        cin >> s >> q >> d;
        int ans = -1;
        int low = 0, high = q;
        int min_cost = -1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (monotonic_best[s][mid] >= d) {
                min_cost = mid;
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        if (min_cost != -1) cout << (q - min_cost) << "\n";
        else cout << "-1\n";
    }

    return 0;
}
