#include "BTree.h"
#include <iostream>
#include <utility>
#include <algorithm>

BTreeNode::BTreeNode(int minDegree, bool leaf) {
    t = minDegree;
    isLeaf = leaf;
    keys = new std::string[2 * t - 1];
    values = new std::vector<FileNode*>[2 * t - 1];
    children = new BTreeNode*[2 * t];
    numKeys = 0;
    for (int i = 0; i < 2 * t; i++) {
        children[i] = nullptr;
    }
}

BTreeNode::~BTreeNode() {
    delete[] keys;
    delete[] values;
    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            delete children[i];
        }
    }
    delete[] children;
}

BTree::BTree(int minDegree) {
    root = nullptr;
    t = minDegree;
}

BTree::~BTree() {
    delete root;
}

std::vector<FileNode*> BTree::search(const std::string& key) const {
    if (root == nullptr) {
        return {};
    }
    int index = -1;
    BTreeNode* foundNode = searchHelper(root, key, index);
    if (foundNode != nullptr) {
        return foundNode->values[index];
    }
    return {};
}

BTreeNode* BTree::searchHelper(BTreeNode* node, const std::string& key, int& index) const {
    int i = 0;
    while (i < node->numKeys && key > node->keys[i]) {
        i++;
    }
    if (i < node->numKeys && node->keys[i] == key) {
        index = i;
        return node;
    }
    if (node->isLeaf) {
        return nullptr;
    }
    return searchHelper(node->children[i], key, index);
}

void BTree::insert(const std::string& key, FileNode* value) {
    int index = -1;
    BTreeNode* existingNode = root ? searchHelper(root, key, index) : nullptr;
    if (existingNode != nullptr) {
        existingNode->values[index].push_back(value);
        return;
    }

    if (root == nullptr) {
        root = new BTreeNode(t, true);
        root->keys[0] = key;
        root->values[0].push_back(value);
        root->numKeys = 1;
        return;
    }

    if (root->numKeys == 2 * t - 1) {
        BTreeNode* s = new BTreeNode(t, false);
        s->children[0] = root;
        splitChild(s, 0, root);
        int i = 0;
        if (s->keys[0] < key) {
            i++;
        }
        insertNonFull(s->children[i], key, value);
        root = s;
    } else {
        insertNonFull(root, key, value);
    }
}

void BTree::splitChild(BTreeNode* parent, int i, BTreeNode* child) {
    BTreeNode* z = new BTreeNode(child->t, child->isLeaf);
    z->numKeys = t - 1;

    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = child->keys[j + t];
        z->values[j] = std::move(child->values[j + t]);
    }

    if (!child->isLeaf) {
        for (int j = 0; j < t; j++) {
            z->children[j] = child->children[j + t];
            child->children[j + t] = nullptr;
        }
    }

    child->numKeys = t - 1;

    for (int j = parent->numKeys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = z;

    for (int j = parent->numKeys - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = std::move(parent->values[j]);
    }

    parent->keys[i] = child->keys[t - 1];
    parent->values[i] = std::move(child->values[t - 1]);
    parent->numKeys++;
}

void BTree::insertNonFull(BTreeNode* node, const std::string& key, FileNode* value) {
    int i = node->numKeys - 1;

    if (node->isLeaf) {
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = std::move(node->values[i]);
            i--;
        }
        node->keys[i + 1] = key;
        node->values[i + 1].clear();
        node->values[i + 1].push_back(value);
        node->numKeys++;
    } else {
        while (i >= 0 && node->keys[i] > key) {
            i--;
        }
        i++;

        if (node->children[i]->numKeys == 2 * t - 1) {
            splitChild(node, i, node->children[i]);
            if (node->keys[i] < key) {
                i++;
            }
        }
        insertNonFull(node->children[i], key, value);
    }
}

void BTree::getKeysWithPrefixHelper(BTreeNode* node, const std::string& prefix, std::vector<std::string>& results) const {
    if (node == nullptr) return;
    for (int i = 0; i < node->numKeys; ++i) {
        if (node->keys[i].rfind(prefix, 0) == 0) {
            results.push_back(node->keys[i]);
        }
    }
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; ++i) {
            getKeysWithPrefixHelper(node->children[i], prefix, results);
        }
    }
}

std::vector<std::string> BTree::getKeysWithPrefix(const std::string& prefix) const {
    std::vector<std::string> results;
    getKeysWithPrefixHelper(root, prefix, results);
    std::sort(results.begin(), results.end());
    return results;
}
