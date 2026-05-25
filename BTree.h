#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <vector>
#include "FileNode.h"

/**
 * @brief Represents a node in the custom B-Tree.
 *
 * EDUCATIONAL DESIGN:
 * A B-Tree is a self-balancing search tree designed to store sorted data and allow
 * searches, sequential access, insertions, and deletions in logarithmic time.
 * In a B-Tree of minimum degree t:
 * - Every node (except root) must contain at least t-1 keys.
 * - Every node can contain at most 2t-1 keys.
 * - An internal node with k keys has exactly k+1 children.
 *
 * Here:
 * - Each key is a filename (std::string).
 * - Each key maps to a std::vector<FileNode*> to support files with identical names
 *   in different directories (multi-value index).
 */
struct BTreeNode {
    int t;                              // Minimum degree (defines bounds for number of keys/children)
    std::string* keys;                  // Dynamically allocated array of keys (filenames)
    std::vector<FileNode*>* values;     // Dynamically allocated array of lists (pointers to FileNodes)
    BTreeNode** children;               // Dynamically allocated array of child node pointers
    int numKeys;                        // Current number of keys in this node
    bool isLeaf;                        // True if this is a leaf node (no children), false otherwise

    /**
     * @brief Constructor for B-Tree Node.
     * @param minDegree The minimum degree of the tree.
     * @param leaf True if this node is a leaf.
     */
    BTreeNode(int minDegree, bool leaf);

    /**
     * @brief Destructor that cleans up arrays and children recursively.
     */
    ~BTreeNode();
};

/**
 * @brief Custom B-Tree index for O(log n) file search by filename.
 */
class BTree {
private:
    BTreeNode* root;                    // Pointer to the root of the B-Tree
    int t;                              // Minimum degree of the B-Tree

    /**
     * @brief Split a child node of a parent node when it becomes full.
     * @param parent The parent node.
     * @param i Index of the child node to split in the parent's children array.
     * @param child The full child node to split.
     */
    void splitChild(BTreeNode* parent, int i, BTreeNode* child);

    /**
     * @brief Insert a key-value pair into a node that is guaranteed not to be full.
     * @param node The node where insertion will occur.
     * @param key The filename key to insert.
     * @param value The FileNode pointer to index.
     */
    void insertNonFull(BTreeNode* node, const std::string& key, FileNode* value);

    /**
     * @brief Helper to recursively search for a key in the B-Tree.
     * @param node Current node in traversal.
     * @param key Filename key to search.
     * @param index Output parameter to store key index if found.
     * @return BTreeNode* Node containing the key, or nullptr if not found.
     */
    BTreeNode* searchHelper(BTreeNode* node, const std::string& key, int& index) const;

    /**
     * @brief Helper to recursively collect all B-Tree keys matching a prefix.
     */
    void getKeysWithPrefixHelper(BTreeNode* node, const std::string& prefix, std::vector<std::string>& results) const;

public:
    /**
     * @brief Construct B-Tree with a given minimum degree.
     * @param minDegree Minimum degree t (t >= 2). A higher t means wider nodes.
     */
    BTree(int minDegree = 3);

    /**
     * @brief Destroy B-Tree. Deleting the root triggers recursive destruction.
     */
    ~BTree();

    /**
     * @brief Insert a file node into the search index.
     * @param key The filename of the node.
     * @param value The pointer to the FileNode.
     */
    void insert(const std::string& key, FileNode* value);

    /**
     * @brief Search for all file nodes matching a given filename.
     * @param key The filename to look up.
     * @return std::vector<FileNode*> A vector of pointers to matching nodes (empty if none).
     */
    std::vector<FileNode*> search(const std::string& key) const;

    /**
     * @brief Get the root node of the B-Tree (for visualization).
     */
    BTreeNode* getRoot() const { return root; }

    /**
     * @brief Gets all keys in the B-Tree that start with a prefix (for autocomplete).
     */
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;
};

#endif // BTREE_H
