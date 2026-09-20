import sys

def analyze_experiment(base_weight, observations, threshold=0.0, metadata=None):
    """
    筛选大于 threshold 的观测值，计算平均值后乘以 base_weight。
    metadata 表示实验名称，仅用于记录，不参与计算。
    """
    valid = [x for x in observations if x > threshold]
    if not valid:
        return 0.0
    return (sum(valid) / len(valid)) * base_weight


def main():
    lines = sys.stdin.read().splitlines()
    if not lines:
        return

    n = int(lines[0].strip())
    output = []
    idx = 1

    for _ in range(n):
        # 跳过可能的空行
        while idx < len(lines) and not lines[idx].strip():
            idx += 1
        if idx >= len(lines):
            break

        parts = lines[idx].strip().split()
        idx += 1

        name = parts[0]
        base_weight = float(parts[1])
        threshold = float(parts[2])
        observations = [float(x) for x in parts[3:]]

        result = analyze_experiment(base_weight, observations, threshold, name)
        output.append(f"{name}: {result:.2f}")

    sys.stdout.write("\n".join(output))


if __name__ == "__main__":
    main()