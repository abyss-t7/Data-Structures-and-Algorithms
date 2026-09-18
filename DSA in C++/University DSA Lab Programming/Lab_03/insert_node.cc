#include <iostream>
using namespace std;

struct Node {
    int data;
    Node* next;
};

int main() {

    Node* x = new Node;
    Node* y = new Node;
    Node* z = new Node;

    x->data = 10;
    y->data = 20;
    z->data = 30;

    x->next = y;
    y->next = z;
    z->next = NULL;

    Node* head = x;

    Node* newNode = new Node;

    newNode->data = 15;

    newNode->next = head->next;
    head->next = newNode;

    Node* stack = head;

    while (stack != nullptr) {
        cout << stack->data << " -> ";
        stack = stack->next;
    }

    cout << "NULL";

    return 0;
}
//  10 -> 15 -> 20 -> 30 -> NULL