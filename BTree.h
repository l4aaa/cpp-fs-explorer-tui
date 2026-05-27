#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <vector>
#include "FileNode.h"

struct BTreeNode {
    int t;
    std::string* keys;
    std::vector<FileNode*>* values;
    BTreeNode** children;
    int numKeys;
    bool isLeaf;

    BTreeNode(int minDegree, bool leaf);
    ~BTreeNode();
};

class BTree {
private:
    BTreeNode* root;
    int t;

    void splitChild(BTreeNode* parent, int i, BTreeNode* child);
    void insertNonFull(BTreeNode* node, const std::string& key, FileNode* value);
    BTreeNode* searchHelper(BTreeNode* node, const std::string& key, int& index) const;
    void getKeysWithPrefixHelper(BTreeNode* node, const std::string& prefix, std::vector<std::string>& results) const;

public:
    BTree(int minDegree = 3);
    ~BTree();

    void insert(const std::string& key, FileNode* value);
    std::vector<FileNode*> search(const std::string& key) const;
    BTreeNode* getRoot() const { return root; }
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;
};

#endif
