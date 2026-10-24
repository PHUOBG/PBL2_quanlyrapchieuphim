#pragma once
#include <string>
using namespace std;

// Linked list node để lưu children
struct ChildNode {
    unsigned char key;       // Ký tự (byte UTF-8)
    class TrieNode* child;   // Con trỏ tới TrieNode con
    ChildNode* next;         // Con trỏ tới node tiếp theo trong list
    
    ChildNode(unsigned char k, TrieNode* c);
};

class TrieNode {
public: 
    bool isEnd;
    ChildNode* childrenHead;  // Đầu danh sách liên kết các children
    
    TrieNode();
    ~TrieNode();
    
    TrieNode* findChild(unsigned char key);
    TrieNode* addChild(unsigned char key);
};

// Simple dynamic array để thay thế vector<string>
class StringArray {
public:
    char** data;      // Mảng các con trỏ string
    int size;         // Số lượng phần tử hiện tại
    int capacity;     // Dung lượng
    
    StringArray();
    ~StringArray();
    
    void add(const string& str);
    void add(const char* str);
    const char* get(int index) const;
    void clear();
};

class Trie {
private:
    TrieNode* root;
    void dfs(TrieNode* node, char* current, int currentLen, StringArray& results, int limit);
    
public: 
    Trie();
    ~Trie();
    void insert(const string& word);
    bool search(const string& word);
    bool startsWith(const string& prefix);
    StringArray getSuggestions(const string& prefix, int limit = 5);
};

