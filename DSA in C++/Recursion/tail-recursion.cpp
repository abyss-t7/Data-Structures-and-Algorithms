#include <iostream>
using namespace std;

void tail(int n)
{

    if (n == 0) 
        return ; 

    tail(n - 1);
    cout << n << endl;
}

int main()
{

    int u;
    cout<<"Enter value for u: "; cin >> u;
    
    tail(u);
}
/*
Enter value for u: 9
1
2
3
4
5
6
7
8
9
*/