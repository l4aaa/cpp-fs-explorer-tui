#include "BTree.h"
#include <iostream>
#include <utility>
#include <algorithm>

// ============================================================================
// BTreeNode Implementation
// ============================================================================

BTreeNode::BTreeNode(int minDegree, bool leaf) {
    t = minDegree;
    isLeaf = leaf;

    // Allocate keys array: A B-Tree node can contain at most (2t - 1) keys.
    keys = new std::string[2 * t - 1];

    // Allocate values array: Each key corresponds to a vector storing pointers
    // to FileNodes with that filename (to handle duplicates in different folders).
    values = new std::vector<FileNode*>[2 * t - 1];

    // Allocate children array: A node can have at most 2t child pointers.
    children = new BTreeNode*[2 * t];

    numKeys = 0;

    // Initialize all child pointers to nullptr to prevent garbage address access.
    for (int i = 0; i < 2 * t; i++) {
        children[i] = nullptr;
    }
}

BTreeNode::~BTreeNode() {
    // Deallocate the arrays created in the constructor.
    delete[] keys;
    delete[] values;

    // EDUCATIONAL COMMENT on Destructor recursion:
    // If this node is an internal node (not a leaf), it owns its children.
    // Deleting the children pointers recursively destroys the subtrees.
    // An internal node with 'numKeys' keys has exactly 'numKeys + 1' children.
    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            delete children[i];
        }
    }
    delete[] children;
}

// ============================================================================
// BTree Implementation
// ============================================================================

BTree::BTree(int minDegree) {
    root = nullptr;
    t = minDegree;
}

BTree::~BTree() {
    // Deleting the root triggers the recursive ~BTreeNode() destructor chain,
    // safely deallocating all nodes in the tree without memory leaks.
    delete root;
}

/**
 * Time Complexity of BTree::search:
 * O(t * log_t n) where n is the number of keys and t is the minimum degree.
 * Since t is a small constant, this simplifies to O(log n).
 *
 * Space Complexity:
 * O(log_t n) recursion stack depth.
 */
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
    // Perform a linear search inside the current node keys.
    // Since keys are sorted inside each node, we could use binary search,
    // but linear search is fast enough and simple for small t (e.g., t = 3).
    while (i < node->numKeys && key > node->keys[i]) {
        i++;
    }

    // If the key matches node->keys[i], search is successful.
    if (i < node->numKeys && node->keys[i] == key) {
        index = i;
        return node;
    }

    // If the key is not found here and this is a leaf node, the key does not exist.
    if (node->isLeaf) {
        return nullptr;
    }

    // Otherwise, descend recursively to the appropriate child.
    // The key is smaller than node->keys[i], so it resides in child 'i'.
    return searchHelper(node->children[i], key, index);
}

/**
 * Time Complexity of BTree::insert:
 * O(t * log_t n) - We traverse down the tree once. Any node we encounter that
 * is full (2t - 1 keys) is immediately split, ensuring we never have to backtrack.
 * Thus, insertion runs in O(log n) time.
 */
void BTree::insert(const std::string& key, FileNode* value) {
    // 1. DUP CHECK: First search if the filename already exists in the B-Tree index.
    // Since we support duplicate filenames in different directories, we store
    // them in a list (std::vector) corresponding to a single key.
    int index = -1;
    BTreeNode* existingNode = root ? searchHelper(root, key, index) : nullptr;
    if (existingNode != nullptr) {
        // The filename already exists as a key in the B-Tree.
        // We append the new FileNode pointer to the existing list.
        // Time Complexity: O(log n) to find, and O(1) amortized to append.
        existingNode->values[index].push_back(value);
        return;
    }

    // 2. ROOT INITIALIZATION: If the tree is completely empty.
    if (root == nullptr) {
        root = new BTreeNode(t, true);
        root->keys[0] = key;
        root->values[0].push_back(value);
        root->numKeys = 1;
        return;
    }

    // 3. ROOT SPLIT: If the root is full (contains 2t - 1 keys).
    // The B-Tree must grow in height. We split the root and make a new root.
    if (root->numKeys == 2 * t - 1) {
        // Create a new node that will become the new root.
        // It is not a leaf node because the old root will be its child.
        BTreeNode* s = new BTreeNode(t, false);

        // Make the old root the first child of the new root.
        s->children[0] = root;

        // Split the old root (child 0 of s) and promote the middle key to s.
        splitChild(s, 0, root);

        // s now has two children. Determine which of the two children
        // should receive the new key.
        int i = 0;
        if (s->keys[0] < key) {
            i++;
        }
        insertNonFull(s->children[i], key, value);

        // Update the root pointer to the new root.
        root = s;
    } else {
        // If root is not full, insert directly into the non-full root.
        insertNonFull(root, key, value);
    }
}

/**
 * @brief Splits a child node when it becomes full (contains 2t - 1 keys).
 *
 * EDUCATIONAL COMMENT on splitting:
 * When splitting a node 'child' (which is the i-th child of 'parent'):
 * 1. We create a new sibling node 'z' at the same level.
 * 2. We move the rightmost (t-1) keys and values from 'child' to 'z'.
 * 3. If 'child' is an internal node, we also move the rightmost 't' child pointers to 'z'.
 * 4. We promote the median key (at index t-1) from 'child' to 'parent'.
 * 5. We insert 'z' as the (i+1)-th child of 'parent'.
 *
 * This operation is O(t) due to copying keys and shifting parent pointers,
 * but since t is a small constant, it is O(1) in practice.
 */
void BTree::splitChild(BTreeNode* parent, int i, BTreeNode* child) {
    // Create a new node 'z' of the same leaf status as 'child'.
    BTreeNode* z = new BTreeNode(child->t, child->isLeaf);
    z->numKeys = t - 1;

    // Copy the last (t - 1) keys and values of 'child' to 'z'.
    // The keys are located from index t to 2t - 2 in 'child'.
    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = child->keys[j + t];
        z->values[j] = std::move(child->values[j + t]);
    }

    // If 'child' is not a leaf, copy its last t child pointers to 'z'.
    // Child pointers are located from index t to 2t - 1 in 'child'.
    if (!child->isLeaf) {
        for (int j = 0; j < t; j++) {
            z->children[j] = child->children[j + t];
            // Clear the pointers in child to prevent double-deletion bugs
            // when child is destroyed later.
            child->children[j + t] = nullptr;
        }
    }

    // Reduce the key count in the original child. It now contains t - 1 keys.
    child->numKeys = t - 1;

    // Shift child pointers of parent to the right to make room for 'z' at parent->children[i+1].
    for (int j = parent->numKeys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    // Link new sibling node 'z' to parent.
    parent->children[i + 1] = z;

    // Shift keys and values of parent to make room for the promoted key from 'child'.
    for (int j = parent->numKeys - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = std::move(parent->values[j]);
    }

    // Promote the middle key (index t - 1) of 'child' to 'parent'.
    parent->keys[i] = child->keys[t - 1];
    parent->values[i] = std::move(child->values[t - 1]);
    parent->numKeys++;
}

/**
 * @brief Insert key-value pair into a non-full node.
 *
 * EDUCATIONAL COMMENT:
 * We traverse down the tree, splitting full nodes along the way.
 * This guarantees that when we reach a leaf node, it is not full,
 * so we can insert the new key directly without backtracking up the tree.
 */
void BTree::insertNonFull(BTreeNode* node, const std::string& key, FileNode* value) {
    int i = node->numKeys - 1;

    if (node->isLeaf) {
        // If this is a leaf node, shift existing keys to the right
        // to find the correct sorted insertion index.
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = std::move(node->values[i]);
            i--;
        }

        // Insert the key and value pointer at the correct index.
        node->keys[i + 1] = key;
        node->values[i + 1].clear();
        node->values[i + 1].push_back(value);
        node->numKeys++;
    } else {
        // If this is an internal node, find the child subtree to descend into.
        while (i >= 0 && node->keys[i] > key) {
            i--;
        }
        i++; // The child index is i + 1.

        // If the targeted child is full, split it before descending.
        if (node->children[i]->numKeys == 2 * t - 1) {
            splitChild(node, i, node->children[i]);

            // After the split, the median key moves up to 'node'.
            // Check if the key goes to children[i] or children[i+1].
            if (node->keys[i] < key) {
                i++;
            }
        }
        // Recurse down to the non-full child.
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
