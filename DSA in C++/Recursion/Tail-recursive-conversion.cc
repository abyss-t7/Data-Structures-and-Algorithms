#include <iostream>
using namespace std;

// Helper function carrying the running product in 'acc'
int factorialTailHelper(int n, int acc)
{
    if (n < 0)
        return -1;
    if (n == 0)
        return acc; // Returns accumulator directly at base case

    // Tail call: computation happens in the arguments, not after return
    return factorialTailHelper(n - 1, n * acc);
}

// Clean wrapper function for callers
int factorialTail(int n)
{
    return factorialTailHelper(n, 1);
}

int main() {

    int n;
    cout<< "Enter value for n :"; cin>>n;

    cout<<factorialTailHelper(n, 3);

}
// 18