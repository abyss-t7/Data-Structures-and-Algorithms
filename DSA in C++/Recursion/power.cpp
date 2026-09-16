#include<iostream>
using namespace std;

int power(int n) {

    if (n == 0)
        return 1;
    
    return 2 * power(n-1);

}

int main(){
    int k = 3;

    cout<<power(k)<<endl;

}
/*
how it works?
power(3) = 2 * power(2)
         = 2 * (2 * power(1))
         = 2 * (2 * (2 * power(0)))
         = 2 * (2 * (2 * 1))         <-- Base case returns 1
         = 2 * (2 * 2)
         = 2 * 4
         = 8
*/