# fmt: off
depth_upper_bound = [0,0,1,3,3,5,5,6,6,7,7,8,8,9,9,9,9,10,11,11,11,12,12,12,12,13,13,14,14,14,14,14,14,15,15,16,16,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,20,20,20,20,20,20,20,20]
# fmt: on


def merge_depth(n):
    """
    Returns the depth of the network that merges two sorted lists of size floor(n/2) and ceil(n/2).
    """
    if n <= 1:
        return 0
    half_n = (n + 1) // 2  # ceil(n/2)
    return merge_depth(half_n) + 1


def sort_depth(n):
    """
    Returns the depth of the sorting network for n inputs, using odd-even merge if possible.

    Uses a divide-and-conquer approach:
    1. Sort the first half (depth_upper_bound[ceil(n/2)])
    2. Sort the second half (depth_upper_bound[floor(n/2)])
    3. Merge the two sorted halves (merge_depth(n))

    Args:
        n: Number of inputs to sort

    Returns:
        The total depth required for sorting
    """
    half_n = (n + 1) // 2  # ceil(n/2)
    return depth_upper_bound[half_n] + merge_depth(n)


def main():
    for n in range(1, len(depth_upper_bound)):
        d = merge_depth(n)
        print(f"merge_depth {n}: {d}")
    for n in range(1, len(depth_upper_bound)):
        d = sort_depth(n)
        if d == depth_upper_bound[n]:
            print(f"sort_depth {n}: {d}. ODD-EVEN MERGE")
        if d < depth_upper_bound[n]:
            print(f"sort_depth {n}: {d}. BETTER UPPER BOUND")
        if d > depth_upper_bound[n]:
            print(f"sort_depth {n}: {d}")


if __name__ == "__main__":
    main()
