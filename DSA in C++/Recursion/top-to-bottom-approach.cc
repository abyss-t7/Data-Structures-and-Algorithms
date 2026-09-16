#include <iostream>
#include <vector>
using namespace std;

int fibHelper(int n, vector<int> &memo)
{
    if (n <= 1)
        return n;

    // Return cached result if already calculated
    if (memo[n] != -1)
        return memo[n];

    // Store calculated value in memo vector
    return memo[n] = fibHelper(n - 1, memo) + fibHelper(n - 2, memo);
}

int fibonacciMemo(int n)
{
    if (n < 0)
        return -1;
    vector<int> memo(n + 1, -1); // Initialize cache with -1
    return fibHelper(n, memo);
}

int main()
{
    int n = 45; // Computes instantly compared to naive recursion
    cout << "Fibonacci(" << n << ") = " << fibonacciMemo(n) << endl;
    return 0;
}