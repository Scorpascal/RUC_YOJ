import sys

def main():
    data = sys.stdin.read().strip().split()
    if not data:
        return

    prices = list(map(int, data))
    n = len(prices)

    if n == 1:
        print(prices[0])
        return

    # dp[i-2] 和 dp[i-1]
    prev2 = prices[0]
    prev1 = max(prices[0], prices[1])

    for i in range(2, n):
        cur = max(prev1, prev2 + prices[i])
        prev2, prev1 = prev1, cur

    print(prev1)

if __name__ == "__main__":
    main()