#include <stdio.h>
	
	// 判断n是否是2的幂次
	// 返回1表示是2的幂，返回0表示不是
	int isPowerOfTwo(long long n) {
	// 2的幂的二进制只有一个1，n & (n-1)会消去最后一个1，结果为0
	return (n & (n - 1)) == 0;
	}
	
	int main() {
	int T;
	scanf("%d", &T); // 读取数据组数
	
	while (T--) {
	long long n;
	scanf("%lld", &n); // 读取n（用long long存储1e18级别的数）
	
	if (isPowerOfTwo(n)) {
	printf("1\n"); // 是2的幂，输出1
	} else {
	printf("2\n"); // 不是2的幂，输出2
	}
	}
	
	return 0;
	}
