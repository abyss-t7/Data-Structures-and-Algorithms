#include <iostream>
using namespace std;

void printing(int n)
{

    if (n == 0)
        return;

    cout << n << endl;

    printing(n - 1);
}

int main()
{

    int u;
    cout << "Enter value for u: ";
    cin >> u;

    printing(u);
}
/*
Enter value for u: 6
6
5
4
3
2
1
*/
// aizaz@4752