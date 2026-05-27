#ifndef FILENODE_H
#define FILENODE_H

#include <string>

enum class NodeType {
    FILE,
    DIRECTORY
};

struct FileNode {
    std::string name;
    NodeType type;
    FileNode* parent;
    FileNode* firstChild;
    FileNode* nextSibling;

    FileNode(const std::string& n, NodeType t, FileNode* p = nullptr)
        : name(n), type(t), parent(p), firstChild(nullptr), nextSibling(nullptr) {}

    ~FileNode() {
        delete firstChild;
        delete nextSibling;
    }
};

#endif
