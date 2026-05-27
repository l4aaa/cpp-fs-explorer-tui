#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include "FileNode.h"
#include "BTree.h"

class FileSystem {
private:
    FileNode* root;
    FileNode* currentDir;
    BTree index;

    FileNode* findChild(FileNode* parent, const std::string& name) const;
    bool insertChildSorted(FileNode* parent, FileNode* child);
    std::vector<std::string> parsePath(const std::string& path) const;
    FileNode* resolvePathToNode(const std::string& path) const;
    FileNode* resolveParentDirectory(const std::string& path, std::string& targetName) const;
    void printTreeHelper(FileNode* node, const std::string& prefix, bool isLast) const;

public:
    FileSystem();
    ~FileSystem();

    bool mkdir(const std::string& path);
    bool touch(const std::string& path);
    void ls() const;
    bool cd(const std::string& path);
    void search(const std::string& name) const;
    void tree() const;
    std::string getCurrentPath() const;
    std::string getAbsolutePath(FileNode* node) const;
    std::vector<FileNode*> getCurrentDirChildren() const;
    std::vector<FileNode*> searchNodes(const std::string& name) const;
    FileNode* getCurrentDir() const;
    const BTree& getIndex() const { return index; }
};

#endif
