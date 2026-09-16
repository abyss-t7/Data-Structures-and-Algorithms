# Time & Space Complexity !

```complexity arena
O(1) < O(log n) < O(n) < O(n log n) < O(n²) < O(2ⁿ) < O(n!)
   ↑                                                        ↑
 fastest                                                 slowest
```

## O(1) — Constant time
```
int getFirst(int arr[], int n) {
    return arr[0];   // doesn't matter if n = 5 or n = 5,000,000 — one operation
}
```