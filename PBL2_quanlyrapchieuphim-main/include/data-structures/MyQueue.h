#pragma once
#include <cstddef>
#include <stdexcept>

/**
 * @brief Custom FIFO Queue using Linked Nodes for PBL2
 */
template <typename T>
class MyQueue {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& val) : data(val), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    size_t count;

public:
    MyQueue() : head(nullptr), tail(nullptr), count(0) {}

    ~MyQueue() {
        clear();
    }

    void push(const T& val) {
        Node* newNode = new Node(val);
        if (!tail) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        ++count;
    }

    void pop() {
        if (!head) return;
        Node* temp = head;
        head = head->next;
        if (!head) tail = nullptr;
        delete temp;
        --count;
    }

    T& front() {
        if (!head) throw std::runtime_error("MyQueue is empty");
        return head->data;
    }

    const T& front() const {
        if (!head) throw std::runtime_error("MyQueue is empty");
        return head->data;
    }

    bool empty() const { return count == 0; }
    size_t size() const { return count; }

    void clear() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        count = 0;
    }
};