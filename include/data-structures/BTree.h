#pragma once

#include <cstddef>
#include <utility>
#include "DLL.h"

template <typename Key, typename Value, int MinDegree = 3>
class BTree {
private:
    static constexpr int MAX_KEYS = 2 * MinDegree - 1;
    static constexpr int MAX_CHILDREN = 2 * MinDegree;

    struct Node {
        bool leaf;
        int keyCount;
        Key keys[MAX_KEYS];
        Value values[MAX_KEYS];
        Node* children[MAX_CHILDREN];

        explicit Node(bool isLeaf) : leaf(isLeaf), keyCount(0) {
            for (int i = 0; i < MAX_CHILDREN; ++i) {
                children[i] = nullptr;
            }
        }

        ~Node() {
            for (int i = 0; i <= keyCount; ++i) {
                if (children[i] != nullptr) {
                    delete children[i];
                    children[i] = nullptr;
                }
            }
        }

        void insertKeyValueAt(int pos, const Key& key, const Value& value) {
            for (int i = keyCount; i > pos; --i) {
                keys[i] = keys[i - 1];
                values[i] = values[i - 1];
            }
            keys[pos] = key;
            values[pos] = value;
            ++keyCount;
        }

        void insertChildAt(int pos, Node* child) {
            for (int i = keyCount + 1; i > pos; --i) {
                children[i] = children[i - 1];
            }
            children[pos] = child;
        }

        void removeKeyValueAt(int pos) {
            for (int i = pos; i < keyCount - 1; ++i) {
                keys[i] = keys[i + 1];
                values[i] = values[i + 1];
            }
            --keyCount;
        }

        void removeChildAt(int pos) {
            for (int i = pos; i < keyCount; ++i) {
                children[i] = children[i + 1];
            }
            children[keyCount] = nullptr;
        }
    };

    Node* root;

    void clear(Node* node) {
        delete node;
    }

    void splitChild(Node* parent, int childIndex) {
        Node* fullChild = parent->children[childIndex];
        Node* newChild = new Node(fullChild->leaf);

        const int mid = MinDegree - 1;

        newChild->keyCount = MinDegree - 1;
        for (int i = 0; i < MinDegree - 1; ++i) {
            newChild->keys[i] = fullChild->keys[mid + 1 + i];
            newChild->values[i] = fullChild->values[mid + 1 + i];
        }

        if (!fullChild->leaf) {
            for (int i = 0; i < MinDegree; ++i) {
                newChild->children[i] = fullChild->children[mid + 1 + i];
                fullChild->children[mid + 1 + i] = nullptr;
            }
        }

        Key midKey = fullChild->keys[mid];
        Value midValue = fullChild->values[mid];

        fullChild->keyCount = mid;

        parent->insertChildAt(childIndex + 1, newChild);

        parent->insertKeyValueAt(childIndex, midKey, midValue);
    }

    void insertNonFull(Node* node, const Key& key, const Value& value) {
        int i = node->keyCount - 1;

        if (node->leaf) {
            while (i >= 0 && key < node->keys[i]) {
                --i;
            }
            node->insertKeyValueAt(i + 1, key, value);
        } else {
            while (i >= 0 && key < node->keys[i]) {
                --i;
            }
            ++i;

            if (node->children[i]->keyCount == MAX_KEYS) {
                splitChild(node, i);
                if (key > node->keys[i]) {
                    ++i;
                }
            }
            insertNonFull(node->children[i], key, value);
        }
    }

    void rangeCollect(Node* node, const Key& start, const Key& end, DLL<Value>& out) const {
        if (!node) {
            return;
        }

        int i = 0;
        while (i < node->keyCount && node->keys[i] < start) {
            ++i;
        }

        while (i < node->keyCount && node->keys[i] <= end) {
            if (!node->leaf) {
                if (node->children[i] != nullptr) {
                    rangeCollect(node->children[i], start, end, out);
                }
            }
            out.push_back(node->values[i]);
            ++i;
        }

        if (!node->leaf && i <= node->keyCount && node->children[i] != nullptr) {
            rangeCollect(node->children[i], start, end, out);
        }
    }

    int findKeyIndex(Node* node, const Key& key) const {
        int idx = 0;
        while (idx < node->keyCount && node->keys[idx] < key) {
            ++idx;
        }
        return idx;
    }

    void collectAll(Node* node, DLL<Value>& out) const {
        if (!node) return;
        
        for (int i = 0; i < node->keyCount; ++i) {
            if (!node->leaf) {
                collectAll(node->children[i], out);
            }
            out.push_back(node->values[i]);
        }
        if (!node->leaf) {
            collectAll(node->children[node->keyCount], out);
        }
    }

public:
    BTree() : root(nullptr) {}
    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    BTree(BTree&& other) noexcept : root(other.root) {
        other.root = nullptr;
    }

    BTree& operator=(BTree&& other) noexcept {
        if (this == &other) return *this;
        clear(root);
        root = other.root;
        other.root = nullptr;
        return *this;
    }

    ~BTree() { clear(root); }

    void clear() {
        clear(root);
        root = nullptr;
    }

    bool empty() const { return root == nullptr; }

    void insert(const Key& key, const Value& value) {
        if (!root) {
            root = new Node(true);
            root->keys[0] = key;
            root->values[0] = value;
            root->keyCount = 1;
            return;
        }

        if (root->keyCount == MAX_KEYS) {
            Node* newRoot = new Node(false);
            newRoot->children[0] = root;
            splitChild(newRoot, 0);

            int i = 0;
            if (newRoot->keys[0] < key) {
                i = 1;
            }
            insertNonFull(newRoot->children[i], key, value);
            root = newRoot;
        } else {
            insertNonFull(root, key, value);
        }
    }

    Value* search(const Key& key) {
        Node* current = root;
        while (current != nullptr) {
            int i = findKeyIndex(current, key);
            
            if (i < current->keyCount && !(key < current->keys[i]) && !(current->keys[i] < key)) {
                return &current->values[i];
            }
            
            if (current->leaf) {
                return nullptr;
            }
            
            current = current->children[i];
        }
        return nullptr;
    }

    const Value* search(const Key& key) const {
        const Node* current = root;
        while (current != nullptr) {
            int i = 0;
            while (i < current->keyCount && current->keys[i] < key) {
                ++i;
            }
            
            if (i < current->keyCount && !(key < current->keys[i]) && !(current->keys[i] < key)) {
                return &current->values[i];
            }
            
            if (current->leaf) {
                return nullptr;
            }
            
            current = current->children[i];
        }
        return nullptr;
    }

    DLL<Value> rangeQuery(const Key& start, const Key& end) const {
        DLL<Value> results;
        rangeCollect(root, start, end, results);
        return results;
    }

    DLL<Value> getAllValues() const {
        DLL<Value> results;
        collectAll(root, results);
        return results;
    }

    bool contains(const Key& key) const {
        return search(key) != nullptr;
    }
};