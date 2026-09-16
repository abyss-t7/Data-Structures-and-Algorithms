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

## O(n) — Linear time

Operations grow directly with input size — one loop, one pass.

```
int sum(int arr[], int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {   // runs exactly n times
        total += arr[i];
    }
    return total;
}
```

## O(n²) — Quadratic time

A loop inside a loop, both tied to n.

```
void printAllPairs(int arr[], int n) {
    for (int i = 0; i < n; i++) {        // n times
        for (int j = 0; j < n; j++) {    // × n times again
            cout << arr[i] << "," << arr[j] << " ";
        }
    }
}
n iterations of the outer loop, each running the inner loop n times → n × n = n² total operations. Double n here, and work goes up 4x, not 2x — that's what makes O(n²) dangerous at scale.
```

## O(log n) — Logarithmic time

Every step cuts the problem in half. This is binary search's whole trick.

```
int binarySearch(int arr[], int n, int target) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (arr[mid] == target) return mid;
        else if (arr[mid] < target) low = mid + 1;
        else high = mid - 1;   // half the search space thrown away each time
    }
    return -1;
}
With n = 1,000,000, this takes at most ~20 steps (2²⁰ ≈ 1,000,000), not a million. That gap — 20 vs 1,000,000 — is why O(log n) matters so much once data gets large.
```

## O(n log n) — Linearithmic

The complexity of good sorting algorithms (merge sort, quicksort average case)

## Space complexity — it is the same idea, different resource

Instead of counting operations, you count extra memory used as n grows.

```
int sumArray(int arr[], int n) {       // O(1) space — only uses a few variables
    int total = 0;
    for (int i = 0; i < n; i++) total += arr[i];
    return total;
}

int* doubleArray(int arr[], int n) {   // O(n) space — creates a NEW array of size n
    int* result = new int[n];
    for (int i = 0; i < n; i++) result[i] = arr[i] * 2;
    return result;
}
```

# Now test yourself reader!

```
// A
void printTriangle(int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            cout << "*";
        }
        cout << endl;
    }
}

// B
bool isSorted(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i+1]) return false;
    }
    return true;
}

// C
int findInMiddle(int arr[], int n) {
    return arr[n / 2];
}
```
