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

    MyStack(const MyStack& other) : topNode(nullptr), count(0) {
        if (other.topNode) {
            Node* newTop = new Node(other.topNode->data);
            Node* tail = newTop;
            for (Node* cur = other.topNode->next; cur; cur = cur->next) {
                tail->next = new Node(cur->data);
                tail = tail->next;
            }
            topNode = newTop;
            count = other.count;
        }
    }

    MyStack& operator=(const MyStack& other) {
        if (this != &other) {
            clear();
            if (other.topNode) {
                Node* newTop = new Node(other.topNode->data);
                Node* tail = newTop;
                for (Node* cur = other.topNode->next; cur; cur = cur->next) {
                    tail->next = new Node(cur->data);
                    tail = tail->next;
                }
                topNode = newTop;
                count = other.count;
            }
        }
        return *this;
    }

    MyStack(MyStack&& other) noexcept : topNode(other.topNode), count(other.count) {
        other.topNode = nullptr;
        other.count = 0;
    }

    MyStack& operator=(MyStack&& other) noexcept {
        if (this != &other) {
            clear();
            topNode = other.topNode;
            count = other.count;
            other.topNode = nullptr;
            other.count = 0;
        }
        return *this;
    }

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