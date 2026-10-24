#pragma once
#include <utility>
#include <stdexcept>
#include <iterator>
using namespace std;

template <typename T>
class Node {
public:
    T data;
    Node* prev;
    Node* next;
    Node(const T& value) : data(value), prev(nullptr), next(nullptr) {}
    Node(T&& value) : data(move(value)), prev(nullptr), next(nullptr) {}
};

template <typename T>
class DLL {
public:
    // ===== Iterator tuong thich STL =====
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

    private:
        Node<T>* current;
        
    public:
        Iterator(Node<T>* node = nullptr) : current(node) {}
        
        reference operator*() const { return current->data; }
        pointer operator->() const { return &(current->data); }
        
        Iterator& operator++() { 
            if (current) current = current->next; 
            return *this; 
        }
        Iterator operator++(int) { 
            Iterator tmp = *this; 
            ++(*this); 
            return tmp; 
        }
        
        Iterator& operator--() { 
            if (current) current = current->prev; 
            return *this; 
        }
        Iterator operator--(int) { 
            Iterator tmp = *this; 
            --(*this); 
            return tmp; 
        }
        
        bool operator==(const Iterator& other) const { return current == other.current; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
        
        Node<T>* getNode() const { return current; }
    };

    class ConstIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

    private:
        const Node<T>* current;
        
    public:
        ConstIterator(const Node<T>* node = nullptr) : current(node) {}
        ConstIterator(const Iterator& it) : current(it.getNode()) {}
        
        reference operator*() const { return current->data; }
        pointer operator->() const { return &(current->data); }
        
        ConstIterator& operator++() { 
            if (current) current = current->next; 
            return *this; 
        }
        ConstIterator operator++(int) { 
            ConstIterator tmp = *this; 
            ++(*this); 
            return tmp; 
        }
        
        ConstIterator& operator--() { 
            if (current) current = current->prev; 
            return *this; 
        }
        ConstIterator operator--(int) { 
            ConstIterator tmp = *this; 
            --(*this); 
            return tmp; 
        }
        
        bool operator==(const ConstIterator& other) const { return current == other.current; }
        bool operator!=(const ConstIterator& other) const { return current != other.current; }
        
        const Node<T>* getNode() const { return current; }
    };

private:
    Node<T>* head;
    Node<T>* tail;
    int _size;

    void link_back(Node<T>* n) {
        if (!head) head = tail = n;
        else { tail->next = n; n->prev = tail; tail = n; }
        ++_size;
    }

    void link_front(Node<T>* n) {
        if (!head) head = tail = n;
        else { n->next = head; head->prev = n; head = n; }
        ++_size;
    }

public:
    DLL() : head(nullptr), tail(nullptr), _size(0) {}

    DLL(const DLL& other) : head(nullptr), tail(nullptr), _size(0) {
        for (Node<T>* cur = other.head; cur; cur = cur->next) {
            link_back(new Node<T>(cur->data));
        }
    }

    DLL& operator=(const DLL& other) {
        if (this == &other) return *this;
        clear();
        for (Node<T>* cur = other.head; cur; cur = cur->next) {
            link_back(new Node<T>(cur->data));
        }
        return *this;
    }

    DLL(DLL&& other) noexcept
        : head(other.head), tail(other.tail), _size(other._size) {
        other.head = other.tail = nullptr;
        other._size = 0;
    }

    DLL& operator=(DLL&& other) noexcept {
        if (this == &other) return *this;
        clear();
        head = other.head; tail = other.tail; _size = other._size;
        other.head = other.tail = nullptr; other._size = 0;
        return *this;
    }

    ~DLL() { clear(); }

    // ===== Ham kich thuoc kieu STL =====
    bool empty() const { return _size == 0; }
    bool isEmpty() const { return _size == 0; }  // Giu de tuong thich cu
    int size() const { return _size; }
    int getSize() const { return _size; }  // Giu de tuong thich cu

    // ===== Ho tro iterator kieu STL =====
    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(head); }
    ConstIterator end() const { return ConstIterator(nullptr); }
    ConstIterator cbegin() const { return ConstIterator(head); }
    ConstIterator cend() const { return ConstIterator(nullptr); }

    // ===== Truy cap dau va cuoi =====
    T& front() { 
        if (!head) throw out_of_range("DLL front() on empty list");
        return head->data; 
    }
    const T& front() const { 
        if (!head) throw out_of_range("DLL front() on empty list");
        return head->data; 
    }
    T& back() { 
        if (!tail) throw out_of_range("DLL back() on empty list");
        return tail->data; 
    }
    const T& back() const { 
        if (!tail) throw out_of_range("DLL back() on empty list");
        return tail->data; 
    }

    void push_back(const T& value) {
        link_back(new Node<T>(value));
    }

    void push_back(T&& value) {
        link_back(new Node<T>(move(value)));
    }

    void push_front(const T& value) {
        link_front(new Node<T>(value));
    }

    void push_front(T&& value) {
        link_front(new Node<T>(move(value)));
    }

    void pop_front() {
        if (empty()) return;
        Node<T>* temp = head;
        head = head->next;
        if (head) head->prev = nullptr; 
        else tail = nullptr;
        delete temp;
        --_size;
    }

    void pop_back() {
        if (empty()) return;
        Node<T>* temp = tail;
        tail = tail->prev;
        if (tail) tail->next = nullptr; 
        else head = nullptr;
        delete temp;
        --_size;
    }

    // ===== Xoa bang iterator tuong thich STL =====
    Iterator erase(Iterator pos) {
        if (pos == end()) return end();
        Node<T>* cur = pos.getNode();
        Node<T>* nextNode = cur->next;
        
        if (cur->prev) {
            cur->prev->next = cur->next;
        } else {
            head = cur->next;
        }
        if (cur->next) {
            cur->next->prev = cur->prev;
        } else {
            tail = cur->prev;
        }
        delete cur;
        --_size;
        return Iterator(nextNode);
    }

    void removeAt(int index) {
        if (index < 0 || index >= _size) throw out_of_range("DLL removeAt index out of range");
        Node<T>* cur = head;
        for (int i = 0; i < index; ++i) {
            cur = cur->next;
        }
        if (cur->prev) {
            cur->prev->next = cur->next;
        } else {
            head = cur->next;
        }
        if (cur->next) {
            cur->next->prev = cur->prev;
        } else {
            tail = cur->prev;
        }
        delete cur;
        --_size;
    }

    void clear() {
        while (!empty()) pop_front();
    }

    T& operator[](int index) {
        if (index < 0 || index >= _size) throw out_of_range("DLL index out of range");
        Node<T>* cur = head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->data;
    }

    const T& operator[](int index) const {
        if (index < 0 || index >= _size) throw out_of_range("DLL index out of range (const)");
        Node<T>* cur = head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->data;
    }

    Node<T>* getHead() { return head; }
    const Node<T>* getHead() const { return head; }
    Node<T>* getTail() { return tail; }
    const Node<T>* getTail() const { return tail; }
};
