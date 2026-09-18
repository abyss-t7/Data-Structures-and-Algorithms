#include <iostream>
using namespace std;

// Traverse the linked list from head to NULL and display all node values

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

    Node* temp = head;

    while (temp != NULL) {
        cout << temp->data << " -> ";
        temp = temp->next;
    }

    cout << "NULL";

    return 0;
}
// 10 -> 20 -> 30 -> NULL