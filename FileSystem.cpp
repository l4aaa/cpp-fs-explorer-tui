#include "FileSystem.h"
#include <iostream>
#include <sstream>

namespace {
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_DIR   = "\033[1;36m";
    const std::string COLOR_FILE  = "\033[0;32m";
    const std::string COLOR_ERROR = "\033[1;31m";
    const std::string COLOR_INFO  = "\033[1;36m";
}

FileSystem::FileSystem() {
    root = new FileNode("", NodeType::DIRECTORY, nullptr);
    currentDir = root;
}

FileSystem::~FileSystem() {
    delete root;
}

FileNode* FileSystem::findChild(FileNode* parent, const std::string& name) const {
    if (parent == nullptr) return nullptr;

    FileNode* curr = parent->firstChild;

    while (curr != nullptr) {
        if (curr->name == name) {
            return curr;
        }
        curr = curr->nextSibling;
    }
    return nullptr;
}

bool FileSystem::insertChildSorted(FileNode* parent, FileNode* child) {
    if (parent == nullptr || child == nullptr) return false;

    child->parent = parent;

    if (parent->firstChild == nullptr) {
        parent->firstChild = child;
        return true;
    }

    FileNode* prev = nullptr;
    FileNode* curr = parent->firstChild;

    while (curr != nullptr && curr->name < child->name) {
        prev = curr;
        curr = curr->nextSibling;
    }

    if (curr != nullptr && curr->name == child->name) {
        return false;
    }

    if (prev == nullptr) {
        child->nextSibling = parent->firstChild;
        parent->firstChild = child;
    } else {
        child->nextSibling = curr;
        prev->nextSibling = child;
    }

    return true;
}

std::vector<std::string> FileSystem::parsePath(const std::string& path) const {
    std::vector<std::string> segments;
    std::stringstream ss(path);
    std::string item;
    
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            segments.push_back(item);
        }
    }
    return segments;
}

FileNode* FileSystem::resolvePathToNode(const std::string& path) const {
    if (path.empty()) {
        return currentDir;
    }

    FileNode* curr = currentDir;
    size_t startIdx = 0;

    if (path[0] == '/') {
        curr = root;
        startIdx = 1;
    }

    std::vector<std::string> segments = parsePath(path.substr(startIdx));
    for (const std::string& segment : segments) {
        if (segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (curr->parent != nullptr) {
                curr = curr->parent;
            }
            continue;
        }

        FileNode* child = findChild(curr, segment);
        if (child == nullptr) {
            return nullptr;
        }
        curr = child;
    }
    return curr;
}

FileNode* FileSystem::resolveParentDirectory(const std::string& path, std::string& targetName) const {
    if (path.empty()) {
        return nullptr;
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        targetName = path;
        return currentDir;
    }

    std::string parentPath = path.substr(0, lastSlash);
    targetName = path.substr(lastSlash + 1);

    if (parentPath.empty()) {
        return root;
    }

    return resolvePathToNode(parentPath);
}

void FileSystem::printTreeHelper(FileNode* node, const std::string& prefix, bool isLast) const {
    if (node == nullptr) return;

    std::cout << prefix << (isLast ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");

    if (node->type == NodeType::DIRECTORY) {
        std::cout << COLOR_DIR << node->name << "/" << COLOR_RESET << std::endl;
    } else {
        std::cout << COLOR_FILE << node->name << COLOR_RESET << std::endl;
    }

    std::string nextPrefix = prefix + (isLast ? "    " : "\u2502   ");

    FileNode* child = node->firstChild;
    while (child != nullptr) {
        bool childIsLast = (child->nextSibling == nullptr);
        printTreeHelper(child, nextPrefix, childIsLast);
        child = child->nextSibling;
    }
}

bool FileSystem::mkdir(const std::string& path) {
    std::string targetName;
    FileNode* parent = resolveParentDirectory(path, targetName);

    if (parent == nullptr || parent->type != NodeType::DIRECTORY) {
        std::cout << COLOR_ERROR << "Mkdir error: Invalid path or parent directory does not exist." << COLOR_RESET << std::endl;
        return false;
    }

    if (targetName.empty() || targetName == "." || targetName == "..") {
        std::cout << COLOR_ERROR << "Mkdir error: Invalid directory name." << COLOR_RESET << std::endl;
        return false;
    }

    FileNode* newDir = new FileNode(targetName, NodeType::DIRECTORY);

    if (!insertChildSorted(parent, newDir)) {
        delete newDir;
        std::cout << COLOR_ERROR << "Mkdir error: File or directory '" << targetName << "' already exists." << COLOR_RESET << std::endl;
        return false;
    }

    index.insert(targetName, newDir);
    return true;
}

bool FileSystem::touch(const std::string& path) {
    std::string targetName;
    FileNode* parent = resolveParentDirectory(path, targetName);

    if (parent == nullptr || parent->type != NodeType::DIRECTORY) {
        std::cout << COLOR_ERROR << "Touch error: Invalid path or parent directory does not exist." << COLOR_RESET << std::endl;
        return false;
    }

    if (targetName.empty() || targetName == "." || targetName == "..") {
        std::cout << COLOR_ERROR << "Touch error: Invalid file name." << COLOR_RESET << std::endl;
        return false;
    }

    FileNode* newFile = new FileNode(targetName, NodeType::FILE);

    if (!insertChildSorted(parent, newFile)) {
        delete newFile;
        std::cout << COLOR_ERROR << "Touch error: File or directory '" << targetName << "' already exists." << COLOR_RESET << std::endl;
        return false;
    }

    index.insert(targetName, newFile);
    return true;
}

void FileSystem::ls() const {
    FileNode* curr = currentDir->firstChild;
    if (curr == nullptr) {
        std::cout << "(directory is empty)" << std::endl;
        return;
    }

    while (curr != nullptr) {
        if (curr->type == NodeType::DIRECTORY) {
            std::cout << COLOR_DIR << curr->name << "/" << COLOR_RESET << "  ";
        } else {
            std::cout << COLOR_FILE << curr->name << COLOR_RESET << "  ";
        }
        curr = curr->nextSibling;
    }
    std::cout << std::endl;
}

bool FileSystem::cd(const std::string& path) {
    FileNode* target = resolvePathToNode(path);
    if (target == nullptr) {
        std::cout << COLOR_ERROR << "Cd error: Directory '" << path << "' not found." << COLOR_RESET << std::endl;
        return false;
    }
    if (target->type != NodeType::DIRECTORY) {
        std::cout << COLOR_ERROR << "Cd error: '" << path << "' is a file, not a directory." << COLOR_RESET << std::endl;
        return false;
    }
    currentDir = target;
    return true;
}

void FileSystem::search(const std::string& name) const {
    std::vector<FileNode*> matches = index.search(name);
    if (matches.empty()) {
        std::cout << COLOR_ERROR << "Search: '" << name << "' not found in index." << COLOR_RESET << std::endl;
        return;
    }

    std::cout << COLOR_INFO << "Found " << matches.size() << " match(es) for '" << name << "':" << COLOR_RESET << std::endl;
    for (FileNode* match : matches) {
        std::string path = getAbsolutePath(match);
        if (match->type == NodeType::DIRECTORY) {
            std::cout << "  [DIR]  " << COLOR_DIR << path << "/" << COLOR_RESET << std::endl;
        } else {
            std::cout << "  [FILE] " << COLOR_FILE << path << COLOR_RESET << std::endl;
        }
    }
}

void FileSystem::tree() const {
    std::cout << COLOR_DIR << (currentDir == root ? "/" : currentDir->name) << COLOR_RESET << std::endl;

    FileNode* child = currentDir->firstChild;
    while (child != nullptr) {
        bool isLast = (child->nextSibling == nullptr);
        printTreeHelper(child, "", isLast);
        child = child->nextSibling;
    }
}

std::string FileSystem::getAbsolutePath(FileNode* node) const {
    if (node == nullptr) return "";
    if (node->parent == nullptr) return "/";

    std::string path = "";
    FileNode* curr = node;

    while (curr->parent != nullptr) {
        path = "/" + curr->name + path;
        curr = curr->parent;
    }
    return path;
}

std::string FileSystem::getCurrentPath() const {
    return getAbsolutePath(currentDir);
}

std::vector<FileNode*> FileSystem::getCurrentDirChildren() const {
    std::vector<FileNode*> children;
    FileNode* curr = currentDir->firstChild;
    while (curr != nullptr) {
        children.push_back(curr);
        curr = curr->nextSibling;
    }
    return children;
}

std::vector<FileNode*> FileSystem::searchNodes(const std::string& name) const {
    return index.search(name);
}

FileNode* FileSystem::getCurrentDir() const {
    return currentDir;
}
