#pragma once
#include <string>
#include <functional>
#include <type_traits>
using namespace std;

template<typename K, typename V>
class HashTable {
private:
    struct Node {
        K key;
        V value;
        Node* next;
        
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** buckets;
    int capacity;
    int size;
    
    int hashFunction(const K& key) const {
        if constexpr (is_same<K, string>::value) {
            unsigned long hash = 5381;
            for (char c : key) {
                hash = ((hash << 5) + hash) + c;
            }
            return hash % capacity;
        }
        else if constexpr (is_same<K, int>::value) {
            return key % capacity;
        }
        return 0;
    }
    
    void rehash() {
        if ((float)size / capacity < 0.75f) return;
        
        int oldCapacity = capacity;
        Node** oldBuckets = buckets;
        
        capacity *= 2;
        buckets = new Node*[capacity]();
        size = 0;
        
        for (int i = 0; i < oldCapacity; i++) {
            Node* current = oldBuckets[i];
            while (current) {
                insert(current->key, current->value);
                Node* temp = current;
                current = current->next;
                delete temp;
            }
        }
        
        delete[] oldBuckets;
    }
    
public:
    HashTable(int cap = 16) : capacity(cap), size(0) {
        buckets = new Node*[capacity]();
        for (int i = 0; i < capacity; i++) {
            buckets[i] = nullptr;
        }
    }
    
    ~HashTable() {
        for (int i = 0; i < capacity; i++) {
            Node* current = buckets[i];
            while (current) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
        }
        delete[] buckets;
    }

    void clear() {
        for (int i = 0; i < capacity; i++) {
            Node* current = buckets[i];
            while (current) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
            buckets[i] = nullptr;
        }
        size = 0;
    }
    
    void insert(const K& key, const V& value) {
        int index = hashFunction(key);
        
        Node* current = buckets[index];
        while (current) {
            if (current->key == key) {
                current->value = value;
                return;
            }
            current = current->next;
        }
        
        Node* newNode = new Node(key, value);
        newNode->next = buckets[index];
        buckets[index] = newNode;
        size++;
        
        rehash();
    }
    
    V* find(const K& key) {
        int index = hashFunction(key);
        Node* current = buckets[index];
        
        while (current) {
            if (current->key == key) {
                return &current->value;
            }
            current = current->next;
        }
        
        return nullptr;
    }
    
    bool remove(const K& key) {
        int index = hashFunction(key);
        Node* current = buckets[index];
        Node* prev = nullptr;
        
        while (current) {
            if (current->key == key) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    buckets[index] = current->next;
                }
                delete current;
                size--;
                return true;
            }
            prev = current;
            current = current->next;
        }
        
        return false;
    }
    
    bool exists(const K& key) {
        return find(key) != nullptr;
    }
    
    bool contains(const K& key) const {
        int index = hashFunction(key);
        Node* current = buckets[index];
        
        while (current) {
            if (current->key == key) {
                return true;
            }
            current = current->next;
        }
        return false;
    }
    
    V* get(const K& key) {
        return find(key);
    }
    
    int getSize() const { return size; }
    
    void forEach(function<void(const K&, V&)> callback) {
        for (int i = 0; i < capacity; i++) {
            Node* current = buckets[i];
            while (current) {
                callback(current->key, current->value);
                current = current->next;
            }
        }
    }
};