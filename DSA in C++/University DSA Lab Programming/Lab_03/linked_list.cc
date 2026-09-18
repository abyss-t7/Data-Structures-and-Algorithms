#include <iostream>
using namespace std;

struct Node
{
    int data;
    Node *next;
};

int main()
{
    Node *x = new Node;
    Node *y = new Node;
    Node *z = new Node;

    x->data = 10;
    y->data = 20;
    z->data = 30;

    x->next = y;
    y->next = z;
    z->next = nullptr;

    Node *temp = x;

    while (temp != nullptr)
    {
        cout << temp->data << " -> ";
        temp = temp->next;
    }

    cout << "NULL";

    return 0;
}

// 10 -> 20 -> 30 -> NULL