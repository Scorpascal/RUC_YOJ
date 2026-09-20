#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Problem 1115: find first repeated term index for sequence:
// a0 = 1; a_{i+1} = (A * a_i + a_i mod B) mod C
// If repeat index > 2e6, output -1.

int main(void){
	long long A,B,C;
	if(scanf("%lld%lld%lld", &A, &B, &C) != 3) return 0;

	const int LIMIT = 2000000;

	// Build hash table size as power of two >= 4 * LIMIT
	size_t need = (size_t)LIMIT * 4;
	size_t size = 1;
	while(size < need) size <<= 1;

	uint32_t *table = (uint32_t*)malloc(size * sizeof(uint32_t));
	if(!table) return 0;
	// use 0xFFFFFFFF as empty marker
	for(size_t i=0;i<size;i++) table[i] = 0xFFFFFFFFu;

	uint32_t mask = (uint32_t)(size - 1);

	long long a = 1;

	// helper lambda substitute via function pointer not allowed in C; use inline block via function-like macro
	auto_insert_or_find:
	;

	// insert initial a=1
	{ uint32_t v = (uint32_t)a; uint32_t h = (uint32_t)(v * 2654435761u) & mask; while(1){ if(table[h] == 0xFFFFFFFFu){ table[h]=v; break;} if(table[h]==v){ printf("0\n"); free(table); return 0;} h = (h+1)&mask; } }

	for(int i=1;i<=LIMIT;i++){
		long long modpart = (B==0? 0 : a % B);
		long long next = ((A * a) + modpart) % C;
		uint32_t nv = (uint32_t)next;
		uint32_t h = (uint32_t)(nv * 2654435761u) & mask;
		while(1){
			if(table[h] == 0xFFFFFFFFu){ table[h] = nv; break; }
			if(table[h] == nv){ printf("%d\n", i); free(table); return 0; }
			h = (h+1)&mask;
		}
		a = next;
	}

	printf("-1\n");
	free(table);
	return 0;
}

