#include "FileSystem.h"
#include <iostream>
#include <sstream>

// ============================================================================
// ANSI Color Definitions (Optimized for Dark Themes)
// ============================================================================
namespace {
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_DIR   = "\033[1;36m"; // Bold Cyan for directories (highly readable)
    const std::string COLOR_FILE  = "\033[0;32m"; // Green for files
    const std::string COLOR_ERROR = "\033[1;31m"; // Bold Red for errors
    const std::string COLOR_INFO  = "\033[1;36m"; // Bold Cyan for informational headers
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

FileSystem::FileSystem() {
    // EDUCATIONAL COMMENT on Root Initialization:
    // The root node has an empty name, is of type DIRECTORY, and has no parent.
    // It serves as the base of our LCRS binary tree representation.
    root = new FileNode("", NodeType::DIRECTORY, nullptr);
    currentDir = root;
}

FileSystem::~FileSystem() {
    // EDUCATIONAL COMMENT on Destructor:
    // Since root is the top node of the binary tree, calling 'delete root'
    // triggers recursive deletion of its 'firstChild' (left child) and
    // 'nextSibling' (right child), freeing all files and directories in one call.
    delete root;
}

// ============================================================================
// Private Helpers
// ============================================================================

/**
 * Time Complexity of findChild:
 * O(c) where c is the number of direct children in the directory.
 * In LCRS, children are stored as a linked list via right sibling pointers,
 * requiring us to traverse them sequentially to find a match.
 */
FileNode* FileSystem::findChild(FileNode* parent, const std::string& name) const {
    if (parent == nullptr) return nullptr;

    // In LCRS, the left pointer (firstChild) points to the head of the children list.
    FileNode* curr = parent->firstChild;

    // Traverse the sibling chain (right pointers in binary tree).
    while (curr != nullptr) {
        if (curr->name == name) {
            return curr;
        }
        curr = curr->nextSibling;
    }
    return nullptr;
}

/**
 * Time Complexity of insertChildSorted:
 * O(c) where c is the number of children.
 * Inserts the child in sorted alphabetical order, ensuring ls() remains sorted.
 */
bool FileSystem::insertChildSorted(FileNode* parent, FileNode* child) {
    if (parent == nullptr || child == nullptr) return false;

    // Set parent pointer to enable climbing back up (for CD and path resolution).
    child->parent = parent;

    // Case 1: Parent has no children yet.
    if (parent->firstChild == nullptr) {
        parent->firstChild = child;
        return true;
    }

    // Case 2: Sibling list exists. Traverse to insert in alphabetical order.
    FileNode* prev = nullptr;
    FileNode* curr = parent->firstChild;

    while (curr != nullptr && curr->name < child->name) {
        prev = curr;
        curr = curr->nextSibling;
    }

    // Duplicate check: Prevent files/directories with the exact same name in the same folder.
    if (curr != nullptr && curr->name == child->name) {
        return false;
    }

    // Insert the new child node
    if (prev == nullptr) {
        // Insert at the head of the sibling list
        child->nextSibling = parent->firstChild;
        parent->firstChild = child;
    } else {
        // Insert in-between prev and curr
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

/**
 * Time Complexity of resolvePathToNode:
 * O(d * c) where d is the depth of the path (number of segments) and
 * c is the average number of children in each directory along the path.
 * This is very fast and resolves both absolute and relative navigation paths.
 */
FileNode* FileSystem::resolvePathToNode(const std::string& path) const {
    if (path.empty()) {
        return currentDir;
    }

    FileNode* curr = currentDir;
    size_t startIdx = 0;

    // Determine if the path is absolute or relative
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

        // Navigate down to the child matching this segment name
        FileNode* child = findChild(curr, segment);
        if (child == nullptr) {
            return nullptr; // Path segment not found
        }
        curr = child;
    }
    return curr;
}

/**
 * Resolves the parent path of a target node (e.g., "dir1/dir2/file.txt" resolves
 * to node "dir2" and sets targetName to "file.txt").
 */
FileNode* FileSystem::resolveParentDirectory(const std::string& path, std::string& targetName) const {
    if (path.empty()) {
        return nullptr;
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        // No slash: Parent directory is the current directory, target is the name itself.
        targetName = path;
        return currentDir;
    }

    std::string parentPath = path.substr(0, lastSlash);
    targetName = path.substr(lastSlash + 1);

    if (parentPath.empty()) {
        // Path is like "/filename", so parent is root
        return root;
    }

    return resolvePathToNode(parentPath);
}

/**
 * Time Complexity of printTreeHelper:
 * O(N) where N is the total number of nodes in the subdirectory tree.
 * Renders standard ASCII tree branch visualizers.
 */
void FileSystem::printTreeHelper(FileNode* node, const std::string& prefix, bool isLast) const {
    if (node == nullptr) return;

    // Print tree branch structure
    std::cout << prefix << (isLast ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");

    // Color print based on type
    if (node->type == NodeType::DIRECTORY) {
        std::cout << COLOR_DIR << node->name << "/" << COLOR_RESET << std::endl;
    } else {
        std::cout << COLOR_FILE << node->name << COLOR_RESET << std::endl;
    }

    // Construct the prefix for children of the current node.
    // If this node was the last child of its parent, we indent with spaces ("    ").
    // Otherwise, we continue drawing the vertical path line ("\u2502   " -> │   ).
    std::string nextPrefix = prefix + (isLast ? "    " : "\u2502   ");

    // Recursively print children of the node (left child in LCRS binary tree).
    FileNode* child = node->firstChild;
    while (child != nullptr) {
        bool childIsLast = (child->nextSibling == nullptr);
        printTreeHelper(child, nextPrefix, childIsLast);
        child = child->nextSibling;
    }
}

// ============================================================================
// Public Interface Implementation
// ============================================================================

/**
 * Time Complexity of mkdir:
 * O(d * c) to resolve parent path, plus O(c) to insert sorted.
 * Overall, O(d * c) where d is depth of path and c is number of siblings.
 * Also index insertion takes O(log n) where n is total files indexed.
 */
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

    // Create a new directory node.
    FileNode* newDir = new FileNode(targetName, NodeType::DIRECTORY);

    // Insert newDir into parent's child list in alphabetical order.
    if (!insertChildSorted(parent, newDir)) {
        delete newDir; // Deallocate to prevent memory leak if name duplicate.
        std::cout << COLOR_ERROR << "Mkdir error: File or directory '" << targetName << "' already exists." << COLOR_RESET << std::endl;
        return false;
    }

    // Index the folder inside the custom B-Tree for fast search.
    index.insert(targetName, newDir);
    return true;
}

/**
 * Time Complexity of touch:
 * Same as mkdir: O(d * c) path lookup + O(c) sorted insertion + O(log n) indexing.
 */
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

    // Create a new file node.
    FileNode* newFile = new FileNode(targetName, NodeType::FILE);

    // Insert newFile into parent's child list.
    if (!insertChildSorted(parent, newFile)) {
        delete newFile;
        std::cout << COLOR_ERROR << "Touch error: File or directory '" << targetName << "' already exists." << COLOR_RESET << std::endl;
        return false;
    }

    // Index the file inside the B-Tree search index.
    index.insert(targetName, newFile);
    return true;
}

/**
 * Time Complexity of ls:
 * O(c) where c is the number of children inside the directory.
 * Traverses siblings list and prints them.
 */
void FileSystem::ls() const {
    FileNode* curr = currentDir->firstChild;
    if (curr == nullptr) {
        std::cout << "(directory is empty)" << std::endl;
        return;
    }

    while (curr != nullptr) {
        if (curr->type == NodeType::DIRECTORY) {
            // Print directories in bold blue with a trailing slash
            std::cout << COLOR_DIR << curr->name << "/" << COLOR_RESET << "  ";
        } else {
            // Print files in green
            std::cout << COLOR_FILE << curr->name << COLOR_RESET << "  ";
        }
        curr = curr->nextSibling;
    }
    std::cout << std::endl;
}

/**
 * Time Complexity of cd:
 * O(d * c) where d is depth of destination path and c is child count.
 */
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

/**
 * Time Complexity of search:
 * O(log n) B-Tree search + O(d * m) absolute path construction, where
 * n is total indexed files, d is depth, and m is the number of matches.
 * Instantly finds the path of any filename indexed in the file system.
 */
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

/**
 * Time Complexity of tree:
 * O(N) where N is total nodes under currentDir.
 */
void FileSystem::tree() const {
    // Print the starting directory node name
    std::cout << COLOR_DIR << (currentDir == root ? "/" : currentDir->name) << COLOR_RESET << std::endl;

    // Iterate through children of currentDir and recursively print subtrees
    FileNode* child = currentDir->firstChild;
    while (child != nullptr) {
        bool isLast = (child->nextSibling == nullptr);
        printTreeHelper(child, "", isLast);
        child = child->nextSibling;
    }
}

/**
 * Time Complexity of getAbsolutePath:
 * O(d) where d is the depth of the node (climbs up parent pointers).
 */
std::string FileSystem::getAbsolutePath(FileNode* node) const {
    if (node == nullptr) return "";
    if (node->parent == nullptr) return "/"; // Root directory

    std::string path = "";
    FileNode* curr = node;

    // Traverse upwards using parent pointers to build the path.
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
