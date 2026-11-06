#pragma once
#include <cstddef>
#include <stdexcept>

/**
 * @brief Custom LIFO Stack using Linked Nodes for PBL2
 */
template <typename T>
class MyStack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& val, Node* nxt = nullptr) : data(val), next(nxt) {}
    };

    Node* topNode;
    size_t count;

public:
    MyStack() : topNode(nullptr), count(0) {}

    ~MyStack() {
        clear();
    }

    void push(const T& val) {
        topNode = new Node(val, topNode);
        ++count;
    }

    void pop() {
        if (!topNode) return;
        Node* temp = topNode;
        topNode = topNode->next;
        delete temp;
        --count;
    }

    T& top() {
        if (!topNode) throw std::runtime_error("MyStack is empty");
        return topNode->data;
    }

    const T& top() const {
        if (!topNode) throw std::runtime_error("MyStack is empty");
        return topNode->data;
    }

    bool empty() const { return count == 0; }
    size_t size() const { return count; }

    void clear() {
        while (topNode) {
            Node* temp = topNode;
            topNode = topNode->next;
            delete temp;
        }
        count = 0;
    }
};