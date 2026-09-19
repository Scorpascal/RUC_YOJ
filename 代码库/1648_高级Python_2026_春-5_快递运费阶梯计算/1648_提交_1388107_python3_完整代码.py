import sys

def calculate_shipping(w, d, t):
    # 1. 基础运费
    if d <= 100:
        base = 10
    elif d <= 500:
        base = 20
    else:
        base = 30

    # 2. 重量附加费单价
    if w <= 2:
        unit = 5
    elif w <= 10:
        unit = 8
    else:
        unit = 12

    weight_cost = unit * w
    total = base + weight_cost

    # 3. 类型系数
    if t == 1:
        total *= 1.5
        # 4. 特快且重量 > 20 减免 10 元
        if w > 20:
            total -= 10

    return total

def main():
    data = sys.stdin.read().strip().split()
    if not data:
        return

    n = int(data[0])
    idx = 1
    results = []

    for _ in range(n):
        w = float(data[idx])
        d = int(float(data[idx + 1]))   # 距离可能是浮点数，但规则按整数区间
        t = int(data[idx + 2])
        idx += 3

        cost = calculate_shipping(w, d, t)
        results.append(f"{cost:.2f}")

    sys.stdout.write("\n".join(results))

if __name__ == "__main__":
    main()