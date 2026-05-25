#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include "FileNode.h"
#include "BTree.h"

/**
 * @brief Manages the File System Explorer logic.
 *
 * Coordinates:
 * 1. The folder/file tree (represented using Left-Child Right-Sibling binary tree nodes).
 * 2. The custom B-Tree search index mapping filenames to nodes.
 */
class FileSystem {
private:
    FileNode* root;                 // Root directory node ("/")
    FileNode* currentDir;           // Pointer to current working directory node
    BTree index;                    // Custom B-Tree index for O(log n) searches

    /**
     * @brief Finds a direct child by name in a parent directory node.
     * @param parent Pointer to parent directory node.
     * @param name Name of the child node to find.
     * @return Pointer to matching FileNode, or nullptr if not found.
     */
    FileNode* findChild(FileNode* parent, const std::string& name) const;

    /**
     * @brief Inserts a child node into a parent directory's children, keeping siblings sorted.
     * @param parent Pointer to parent directory node.
     * @param child Pointer to the child node to insert.
     * @return True if inserted successfully, false if name already exists.
     */
    bool insertChildSorted(FileNode* parent, FileNode* child);

    /**
     * @brief Parses a path string separated by '/' into individual name segments.
     * @param path The path string to parse.
     * @return Vector of path segment strings.
     */
    std::vector<std::string> parsePath(const std::string& path) const;

    /**
     * @brief Helper to resolve any path (relative or absolute) to its matching node.
     * @param path Path to resolve.
     * @return Pointer to the resolved FileNode, or nullptr if path is invalid.
     */
    FileNode* resolvePathToNode(const std::string& path) const;

    /**
     * @brief Resolves the parent directory node of a path and extracts the target filename.
     * @param path The full path input (e.g. "dir1/subDir/file.txt").
     * @param targetName Output parameter to store the target filename (e.g. "file.txt").
     * @return Pointer to the parent directory FileNode, or nullptr if path is invalid.
     */
    FileNode* resolveParentDirectory(const std::string& path, std::string& targetName) const;

    /**
     * @brief Helper to recursively render a visual tree representation.
     * @param node Current node in traversal.
     * @param prefix String prefix containing branching lines.
     * @param isLast True if this is the last sibling at this level.
     */
    void printTreeHelper(FileNode* node, const std::string& prefix, bool isLast) const;

public:
    /**
     * @brief Construct FileSystem. Initializes root node and B-Tree index.
     */
    FileSystem();

    /**
     * @brief Destructor. Destroys the root node (recursively deleting the entire tree).
     */
    ~FileSystem();

    /**
     * @brief Creates a directory at the specified path.
     * @param path Path of the directory to create (e.g., "newdir" or "path/to/newdir").
     * @return True if successful, false otherwise.
     */
    bool mkdir(const std::string& path);

    /**
     * @brief Creates a file at the specified path.
     * @param path Path of the file to create (e.g., "file.txt" or "path/to/file.txt").
     * @return True if successful, false otherwise.
     */
    bool touch(const std::string& path);

    /**
     * @brief Prints the contents of the current directory in a clean, formatted style.
     */
    void ls() const;

    /**
     * @brief Navigates to a new working directory path.
     * @param path Path (relative, absolute, or "..") to navigate to.
     * @return True if directory changed successfully, false otherwise.
     */
    bool cd(const std::string& path);

    /**
     * @brief Searches for a file or directory name instantly using the B-Tree index.
     * @param name The filename or directory name to search.
     */
    void search(const std::string& name) const;

    /**
     * @brief Prints a visual directory tree structure starting from the current directory.
     */
    void tree() const;

    /**
     * @brief Reconstructs the absolute path string for the current working directory.
     */
    std::string getCurrentPath() const;

    /**
     * @brief Helper to build the absolute path string of any file node in the tree.
     * @param node The FileNode to construct a path for.
     */
    std::string getAbsolutePath(FileNode* node) const;

    /**
     * @brief Gets all child nodes of the current directory.
     */
    std::vector<FileNode*> getCurrentDirChildren() const;

    /**
     * @brief Searches for file nodes by name in the index.
     */
    std::vector<FileNode*> searchNodes(const std::string& name) const;

    /**
     * @brief Gets the current directory pointer.
     */
    FileNode* getCurrentDir() const;

    /**
     * @brief Gets the B-Tree index (for visualization).
     */
    const BTree& getIndex() const { return index; }
};

#endif // FILESYSTEM_H
