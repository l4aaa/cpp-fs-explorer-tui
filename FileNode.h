#ifndef FILENODE_H
#define FILENODE_H

#include <string>

// Enum representing the type of node in our file system.
enum class NodeType {
    FILE,
    DIRECTORY
};

/**
 * @brief Represents a node in the custom file system hierarchy.
 *
 * EDUCATIONAL DESIGN:
 * To represent an arbitrary N-ary tree (where directories can contain an arbitrary
 * number of files/directories) using a custom Binary Tree, we use the
 * Left-Child Right-Sibling (LCRS) representation:
 * - The left pointer ("firstChild") points to the first child of a directory.
 * - The right pointer ("nextSibling") points to the next sibling at the same directory level.
 *
 * This maps a general multi-way tree structure 1:1 onto a binary tree representation,
 * avoiding any built-in list/tree collections and satisfying the binary tree constraint.
 */
struct FileNode {
    std::string name;       // Name of the file or directory
    NodeType type;          // Type of this node (FILE or DIRECTORY)
    FileNode* parent;       // Pointer to the parent directory node (helps with path construction & 'cd ..')
    FileNode* firstChild;   // Left child in LCRS binary tree: points to the first child node inside this directory
    FileNode* nextSibling;  // Right child in LCRS binary tree: points to the next sibling at the same level

    /**
     * @brief Constructor to initialize a FileNode.
     * @param n Name of the node.
     * @param t Type (FILE or DIRECTORY).
     * @param p Parent node pointer.
     */
    FileNode(const std::string& n, NodeType t, FileNode* p = nullptr)
        : name(n), type(t), parent(p), firstChild(nullptr), nextSibling(nullptr) {}

    /**
     * @brief Recursive destructor to clean up memory.
     *
     * In an LCRS binary tree:
     * - Deleting firstChild recursively cleans up all descendants.
     * - Deleting nextSibling recursively cleans up all subsequent siblings.
     */
    ~FileNode() {
        delete firstChild;
        delete nextSibling;
    }
};

#endif // FILENODE_H
