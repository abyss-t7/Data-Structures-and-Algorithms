#include <iostream>
using namespace std;
/*
An iterative loop replaces recursive stack frames with a simple accumulator variable updated in a loop
*/

int factorialIterative(int n)
{
    if (n < 0)
        return -1; 

    int result = 1;
    for (int i = 2; i <= n; ++i)
    {
        result *= i;
    }
    return result;
}
int main()
{
    int n;
    cout<<"Enter any value :";
    cin >> n;
    cout << factorialIterative(n) << endl;
}