#include <iostream>
using namespace std;

int fibonacciIterative(int n) {
    if (n < 0) return -1;
    if (n <= 1) return n;

    int prev2 = 0; // F(i-2)
    int prev1 = 1; // F(i-1)
    int current = 0;

    for (int i = 2; i <= n; ++i) {
        current = prev1 + prev2;
        prev2 = prev1;
        prev1 = current;
    }

    return current;
}

int main() {
    int n = 52;
    cout << "Fibonacci(" << n << ") = " << fibonacciIterative(n) << endl;
    return 0;
}