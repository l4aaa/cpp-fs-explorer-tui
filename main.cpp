#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <csignal>
#include <cstdlib>
#include "FileSystem.h"

const std::string COLOR_RESET     = "\033[0m";
const std::string COLOR_BORDER    = "\033[38;5;244m";
const std::string COLOR_DIR       = "\033[1;36m";
const std::string COLOR_FILE      = "\033[1;32m";
const std::string COLOR_SEL_DIR   = "\033[30;46m";
const std::string COLOR_SEL_FILE  = "\033[30;42m";
const std::string COLOR_HEADER    = "\033[1;30;46m";
const std::string COLOR_LEGEND    = "\033[1;30;43m";

enum Key {
    KEY_UP = 1000,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER = '\n',
    KEY_BACKSPACE = 127,
    KEY_ESC = 27,
    KEY_TAB = 9
};

class TerminalRawMode {
public:
    struct termios orig_termios;
    bool active = false;

    TerminalRawMode() {
        enable();
    }
    
    ~TerminalRawMode() {
        disable();
    }

    void enable() {
        if (active) return;
        if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
            struct termios raw = orig_termios;
            raw.c_lflag &= ~(ECHO | ICANON);
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            active = true;
        }
    }

    void disable() {
        if (active) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            active = false;
            std::cout << "\033[?25h" << std::flush;
        }
    }
};

TerminalRawMode* globalRawMode = nullptr;

void handleSignal(int sig) {
    if (globalRawMode) {
        globalRawMode->disable();
    }
    std::cout << "\r\nTerminal restored. Exiting (signal " << sig << ")...\r\n";
    std::exit(sig);
}

int readKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return 0;
    
    if (c == '\033') {
        char seq[3];
        struct timeval tv = {0, 50000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) return KEY_ESC;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) return KEY_ESC;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return KEY_UP;
                    case 'B': return KEY_DOWN;
                    case 'C': return KEY_RIGHT;
                    case 'D': return KEY_LEFT;
                }
            }
        }
        return KEY_ESC;
    }
    
    if (c == 127 || c == 8) return KEY_BACKSPACE;
    if (c == '\r' || c == '\n') return KEY_ENTER;
    return c;
}

int getTerminalSize(int& rows, int& cols) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0) {
        rows = 24;
        cols = 80;
        return -1;
    }
    rows = w.ws_row;
    cols = w.ws_col;
    return 0;
}

int getVisualWidth(const std::string& str) {
    int length = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        if (c < 0x80) {
            i += 1;
            length += 1;
        } else if (c < 0xE0) {
            i += 2;
            length += 1;
        } else if (c < 0xF0) {
            i += 3;
            length += 1;
        } else {
            i += 4;
            length += 1;
        }
    }
    return length;
}

std::string padOrTruncate(const std::string& str, int width) {
    std::string result = "";
    int currentWidth = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        int charBytes = 1;
        if (c < 0x80) charBytes = 1;
        else if (c < 0xE0) charBytes = 2;
        else if (c < 0xF0) charBytes = 3;
        else charBytes = 4;
        
        if (currentWidth + 1 > width) {
            break;
        }
        
        result += str.substr(i, charBytes);
        currentWidth += 1;
        i += charBytes;
    }
    
    if (currentWidth < width) {
        result += std::string(width - currentWidth, ' ');
    }
    return result;
}

std::string findLongestCommonPrefix(const std::vector<std::string>& matches) {
    if (matches.empty()) return "";
    std::string prefix = matches[0];
    for (size_t i = 1; i < matches.size(); ++i) {
        while (matches[i].find(prefix) != 0) {
            prefix = prefix.substr(0, prefix.length() - 1);
            if (prefix.empty()) return "";
        }
    }
    return prefix;
}

struct TUIItem {
    std::string name;
    bool isDir;
    FileNode* node;
};

struct TreeLine {
    std::string prefix;
    std::string connector;
    std::string pointerType;
    std::string name;
    bool isDir;
    std::string text;
};

enum TUIMode {
    BROWSE,
    INPUT_DIR,
    INPUT_FILE,
    SEARCH_INPUT,
    SEARCH_RESULTS
};

enum VisMode {
    DIR_TREE,
    LCRS_TREE,
    BTREE_INDEX
};

void generateTreeLines(FileNode* node, const std::string& prefix, bool isLast, std::vector<TreeLine>& lines) {
    if (node == nullptr || lines.size() > 100) return;
    
    std::string connector = isLast ? "└── " : "├── ";
    std::string text = prefix + connector + node->name;
    lines.push_back({prefix, connector, "", node->name, node->type == NodeType::DIRECTORY, text});
    
    std::string nextPrefix = prefix + (isLast ? "    " : "│   ");
    FileNode* child = node->firstChild;
    while (child != nullptr) {
        bool childIsLast = (child->nextSibling == nullptr);
        generateTreeLines(child, nextPrefix, childIsLast, lines);
        child = child->nextSibling;
    }
}

void generateLCRSTreeLines(FileNode* node, const std::string& prefix, const std::string& pointerType, bool isLast, std::vector<TreeLine>& lines) {
    if (node == nullptr || lines.size() > 100) return;
    
    std::string displayName = node->name.empty() ? "/" : node->name;
    std::string connector = "";
    std::string text = "";
    
    if (prefix.empty() && pointerType.empty()) {
        text = displayName;
    } else {
        connector = isLast ? "└─" : "├─";
        text = prefix + connector + pointerType + "─► " + displayName;
    }
    
    lines.push_back({prefix, connector, pointerType, displayName, node->type == NodeType::DIRECTORY, text});
    
    std::string nextPrefix = prefix + (isLast ? "       " : "│      ");
    
    struct BinaryChild {
        FileNode* childNode;
        std::string type;
    };
    std::vector<BinaryChild> binChildren;
    if (node->firstChild != nullptr) {
        binChildren.push_back({node->firstChild, "FC"});
    }
    if (node->nextSibling != nullptr) {
        binChildren.push_back({node->nextSibling, "NS"});
    }
    
    for (size_t i = 0; i < binChildren.size(); ++i) {
        bool childIsLast = (i == binChildren.size() - 1);
        generateLCRSTreeLines(binChildren[i].childNode, nextPrefix, binChildren[i].type, childIsLast, lines);
    }
}

void generateBTreeLines(BTreeNode* node, const std::string& prefix, bool isLast, std::vector<TreeLine>& lines) {
    if (node == nullptr || lines.size() > 100) return;
    
    std::string displayName = "[";
    for (int i = 0; i < node->numKeys; ++i) {
        displayName += node->keys[i];
        if (i < node->numKeys - 1) {
            displayName += ", ";
        }
    }
    displayName += "]";
    
    std::string connector = "";
    std::string text = "";
    if (prefix.empty()) {
        text = displayName;
    } else {
        connector = isLast ? "└──► " : "├──► ";
        text = prefix + connector + displayName;
    }
    
    lines.push_back({prefix, connector, "", displayName, true, text});
    
    if (!node->isLeaf) {
        std::string nextPrefix = prefix + (isLast ? "     " : "│    ");
        int numChildren = node->numKeys + 1;
        for (int i = 0; i < numChildren; ++i) {
            if (node->children[i] != nullptr) {
                bool childIsLast = (i == numChildren - 1);
                generateBTreeLines(node->children[i], nextPrefix, childIsLast, lines);
            }
        }
    }
}

void drawTreeLine(const TreeLine& tl, int width) {
    int remainingWidth = width;
    
    if (!tl.prefix.empty() && remainingWidth > 0) {
        int prefW = getVisualWidth(tl.prefix);
        if (prefW <= remainingWidth) {
            std::cout << COLOR_BORDER << tl.prefix;
            remainingWidth -= prefW;
        } else {
            std::cout << COLOR_BORDER << padOrTruncate(tl.prefix, remainingWidth);
            remainingWidth = 0;
        }
    }
    
    if (!tl.connector.empty() && remainingWidth > 0) {
        if (!tl.pointerType.empty()) {
            int connBodyW = getVisualWidth(tl.connector);
            if (connBodyW <= remainingWidth) {
                std::cout << COLOR_BORDER << tl.connector;
                remainingWidth -= connBodyW;
            } else {
                std::cout << COLOR_BORDER << padOrTruncate(tl.connector, remainingWidth);
                remainingWidth = 0;
            }
            
            if (remainingWidth > 0) {
                int ptrW = getVisualWidth(tl.pointerType);
                if (ptrW <= remainingWidth) {
                    if (tl.pointerType == "FC") {
                        std::cout << "\033[1;33m" << tl.pointerType << COLOR_RESET;
                    } else {
                        std::cout << "\033[1;35m" << tl.pointerType << COLOR_RESET;
                    }
                    remainingWidth -= ptrW;
                } else {
                    if (tl.pointerType == "FC") {
                        std::cout << "\033[1;33m" << padOrTruncate(tl.pointerType, remainingWidth) << COLOR_RESET;
                    } else {
                        std::cout << "\033[1;35m" << padOrTruncate(tl.pointerType, remainingWidth) << COLOR_RESET;
                    }
                    remainingWidth = 0;
                }
            }
            
            if (remainingWidth > 0) {
                std::string suffix = "─► ";
                int sufW = getVisualWidth(suffix);
                if (sufW <= remainingWidth) {
                    std::cout << COLOR_BORDER << suffix;
                    remainingWidth -= sufW;
                } else {
                    std::cout << COLOR_BORDER << padOrTruncate(suffix, remainingWidth);
                    remainingWidth = 0;
                }
            }
        } else {
            int connW = getVisualWidth(tl.connector);
            if (connW <= remainingWidth) {
                std::cout << COLOR_BORDER << tl.connector;
                remainingWidth -= connW;
            } else {
                std::cout << COLOR_BORDER << padOrTruncate(tl.connector, remainingWidth);
                remainingWidth = 0;
            }
        }
    }
    
    if (remainingWidth > 0) {
        int nameW = getVisualWidth(tl.name);
        if (nameW <= remainingWidth) {
            if (tl.isDir) {
                std::cout << COLOR_DIR << tl.name << COLOR_RESET;
            } else {
                std::cout << COLOR_FILE << tl.name << COLOR_RESET;
            }
            remainingWidth -= nameW;
            if (remainingWidth > 0) {
                std::cout << std::string(remainingWidth, ' ');
            }
        } else {
            if (tl.isDir) {
                std::cout << COLOR_DIR << padOrTruncate(tl.name, remainingWidth) << COLOR_RESET;
            } else {
                std::cout << COLOR_FILE << padOrTruncate(tl.name, remainingWidth) << COLOR_RESET;
            }
            remainingWidth = 0;
        }
    }
}

void drawRightLine(const std::string& text, bool isSelected, bool isDir, int width) {
    std::string indicator = isSelected ? "> " : "  ";
    std::string itemText = indicator + text;
    std::string paddedText = padOrTruncate(itemText, width);
    
    if (isSelected) {
        if (isDir) {
            std::cout << COLOR_SEL_DIR << paddedText << COLOR_RESET;
        } else {
            std::cout << COLOR_SEL_FILE << paddedText << COLOR_RESET;
        }
    } else {
        if (isDir) {
            std::cout << COLOR_DIR << paddedText << COLOR_RESET;
        } else {
            std::cout << COLOR_FILE << paddedText << COLOR_RESET;
        }
    }
}

int main() {
    TerminalRawMode rawMode;
    globalRawMode = &rawMode;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    FileSystem fs;
    fs.mkdir("documents");
    fs.mkdir("photos");
    fs.mkdir("downloads");
    fs.touch("documents/resume.pdf");
    fs.touch("documents/notes.txt");
    fs.touch("photos/beach.png");
    fs.touch("downloads/installer.sh");

    TUIMode mode = BROWSE;
    VisMode visMode = DIR_TREE;
    int selectedIndex = 0;
    int scrollOffset = 0;
    std::string inputBuffer = "";
    std::string statusMessage = "";
    std::string searchQuery = "";
    std::string originalSearchPrefix = "";
    int autocompleteCycleIndex = -1;
    std::vector<FileNode*> searchResults;
    std::vector<TUIItem> items;

    while (true) {
        int rows, cols;
        if (getTerminalSize(rows, cols) < 0) {
            rows = 24; cols = 80;
        }

        items.clear();
        FileNode* currentDir = fs.getCurrentDir();
        
        FileNode* rootNode = currentDir;
        while (rootNode->parent != nullptr) {
            rootNode = rootNode->parent;
        }

        if (mode == SEARCH_RESULTS) {
            for (FileNode* match : searchResults) {
                std::string path = fs.getAbsolutePath(match);
                items.push_back({path, match->type == NodeType::DIRECTORY, match});
            }
        } else {
            if (currentDir != rootNode) {
                items.push_back({"..", true, nullptr});
            }
            std::vector<FileNode*> children = fs.getCurrentDirChildren();
            for (FileNode* child : children) {
                items.push_back({child->name, child->type == NodeType::DIRECTORY, child});
            }
        }

        int activeSize = items.size();
        if (activeSize == 0) {
            selectedIndex = 0;
        } else {
            if (selectedIndex >= activeSize) selectedIndex = activeSize - 1;
            if (selectedIndex < 0) selectedIndex = 0;
        }

        std::vector<TreeLine> treeLines;
        if (visMode == DIR_TREE) {
            std::string treeRootName = (currentDir->parent == nullptr) ? "/" : currentDir->name + "/";
            std::string namePart = currentDir->parent == nullptr ? "/" : currentDir->name + "/";
            treeLines.push_back({"", "", "", namePart, true, treeRootName});
            
            FileNode* child = currentDir->firstChild;
            while (child != nullptr) {
                bool isLast = (child->nextSibling == nullptr);
                generateTreeLines(child, "", isLast, treeLines);
                child = child->nextSibling;
            }
        } else if (visMode == LCRS_TREE) {
            FileNode* rootNode = currentDir;
            while (rootNode->parent != nullptr) {
                rootNode = rootNode->parent;
            }
            generateLCRSTreeLines(rootNode, "", "", true, treeLines);
        } else if (visMode == BTREE_INDEX) {
            BTreeNode* btreeRoot = fs.getIndex().getRoot();
            if (btreeRoot == nullptr) {
                treeLines.push_back({"", "", "", "(B-Tree is empty)", false, "(B-Tree is empty)"});
            } else {
                generateBTreeLines(btreeRoot, "", true, treeLines);
            }
        }

        int contentHeight = rows - 5;
        if (contentHeight < 1) contentHeight = 1;

        int maxTreeLineWidth = 0;
        for (const auto& tl : treeLines) {
            int w = getVisualWidth(tl.text);
            if (w > maxTreeLineWidth) {
                maxTreeLineWidth = w;
            }
        }

        int leftWidth = maxTreeLineWidth + 3;
        if (leftWidth < 20) leftWidth = 20;
        int maxAllowedLeft = cols / 2;
        if (maxAllowedLeft < 20) maxAllowedLeft = 20;
        if (leftWidth > maxAllowedLeft) leftWidth = maxAllowedLeft;
        int rightWidth = cols - leftWidth - 2;

        if (selectedIndex >= scrollOffset + contentHeight) {
            scrollOffset = selectedIndex - contentHeight + 1;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }

        std::cout << "\033[?25l\033[H";

        if (rows < 12 || cols < 40) {
            std::cout << "\033[2J";
            std::cout << "Terminal window is too small. Please resize!\r\n";
            std::cout << "Current: " << cols << "x" << rows << "\r\n";
            std::cout << "Minimum required: 40x12\r\n";
            std::cout << std::flush;
            usleep(100000);
            continue;
        }

        std::string title = " 📁 C++ File System Explorer (TUI Mode) ";
        std::string pathText = " Location: " + fs.getCurrentPath() + " ";
        std::string header = COLOR_HEADER + title;
        int remaining = cols - title.length();
        if (remaining > (int)pathText.length()) {
            header += std::string(remaining - pathText.length(), ' ') + pathText;
        } else {
            header += std::string(remaining > 0 ? remaining : 0, ' ');
        }
        header += COLOR_RESET + "\r\n";
        std::cout << header;

        std::string leftLabel = "";
        if (visMode == DIR_TREE) {
            leftLabel = "── Tree View (Dir) ";
        } else if (visMode == LCRS_TREE) {
            leftLabel = "── LCRS Binary Tree ";
        } else if (visMode == BTREE_INDEX) {
            leftLabel = "── B-Tree Index ";
        }
        
        std::string leftBorder = leftLabel;
        int leftLabelWidth = getVisualWidth(leftLabel);
        if (leftLabelWidth < leftWidth) {
            for (int i = 0; i < leftWidth - leftLabelWidth; ++i) {
                leftBorder += "─";
            }
        } else {
            leftBorder = padOrTruncate(leftLabel, leftWidth);
        }

        std::string rightLabel = "── Folder Contents ";
        if (mode == SEARCH_RESULTS) {
            rightLabel = "── Search Results ";
        }
        std::string rightBorder = rightLabel;
        int rightLabelWidth = getVisualWidth(rightLabel);
        if (rightLabelWidth < rightWidth) {
            for (int i = 0; i < rightWidth - rightLabelWidth; ++i) {
                rightBorder += "─";
            }
        } else {
            rightBorder = padOrTruncate(rightLabel, rightWidth);
        }

        std::cout << COLOR_BORDER << leftBorder << "┬" << rightBorder << COLOR_RESET << "\r\n";

        for (int r = 0; r < contentHeight; ++r) {
            if (r < (int)treeLines.size()) {
                drawTreeLine(treeLines[r], leftWidth);
            } else {
                std::cout << std::string(leftWidth, ' ');
            }

            std::cout << COLOR_BORDER << "│" << COLOR_RESET;

            int itemIndex = r + scrollOffset;
            if (itemIndex < activeSize) {
                drawRightLine(items[itemIndex].name, itemIndex == selectedIndex, items[itemIndex].isDir, rightWidth);
            } else {
                std::cout << std::string(rightWidth, ' ');
            }
            std::cout << "\r\n";
        }

        std::string bottomBorder = "";
        for (int i = 0; i < leftWidth; ++i) bottomBorder += "─";
        bottomBorder += "┴";
        for (int i = 0; i < rightWidth; ++i) bottomBorder += "─";
        std::cout << COLOR_BORDER << bottomBorder << COLOR_RESET << "\r\n";

        std::string promptLine = "";
        if (mode == INPUT_DIR) {
            promptLine = " 📂 Create Directory: " + inputBuffer + "█";
        } else if (mode == INPUT_FILE) {
            promptLine = " 📄 Create File: " + inputBuffer + "█";
        } else if (mode == SEARCH_INPUT) {
            std::string queryPrefix = (autocompleteCycleIndex != -1) ? originalSearchPrefix : inputBuffer;
            std::vector<std::string> suggestions = queryPrefix.empty() ? std::vector<std::string>{} : fs.getIndex().getKeysWithPrefix(queryPrefix);
            
            std::string sugPart = "";
            if (!suggestions.empty()) {
                sugPart = "  [Tab: ";
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    if ((int)i == autocompleteCycleIndex) {
                        sugPart += ">" + suggestions[i] + "<";
                    } else {
                        sugPart += suggestions[i];
                    }
                    if (i < suggestions.size() - 1) {
                        sugPart += ", ";
                    }
                }
                sugPart += "]";
            }
            promptLine = " 🔍 Search (exact filename): " + inputBuffer + "█" + sugPart;
        } else if (mode == SEARCH_RESULTS) {
            promptLine = " 🔎 Search Results for \"" + searchQuery + "\" | Matches: " + std::to_string(items.size()) + " | [Enter] Jump to location";
        } else {
            if (!statusMessage.empty()) {
                promptLine = " " + statusMessage;
            } else if (items.empty()) {
                promptLine = " (empty directory)";
            } else {
                TUIItem sel = items[selectedIndex];
                if (sel.node == nullptr) {
                    promptLine = " Parent Directory (..)";
                } else {
                    promptLine = " Selected: " + fs.getAbsolutePath(sel.node) + " [" + (sel.isDir ? "DIR" : "FILE") + "]";
                }
            }
        }
        std::cout << padOrTruncate(promptLine, cols) << "\r\n";

        std::string legend = "";
        if (mode == SEARCH_RESULTS) {
            legend = " [↑/↓] Navigate Results  [Enter] Go to Folder  [Esc] Back to Browse";
        } else if (mode == INPUT_DIR || mode == INPUT_FILE || mode == SEARCH_INPUT) {
            legend = " [Enter] Confirm  [Esc] Cancel";
        } else {
            legend = " [↑/↓] Select  [Enter] Open/Cd  [m] Mkdir  [t] Touch  [s] Search  [v] Vis Mode  [q/Esc] Quit";
        }
        std::cout << COLOR_LEGEND << padOrTruncate(legend, cols) << COLOR_RESET << std::flush;

        if (mode == BROWSE) {
            statusMessage = "";
        }

        int key = readKey();
        if (key == 0) continue;

        if (mode == INPUT_DIR || mode == INPUT_FILE || mode == SEARCH_INPUT) {
            if (key == KEY_ENTER) {
                if (mode == INPUT_DIR) {
                    if (!inputBuffer.empty()) {
                        bool success = fs.mkdir(inputBuffer);
                        if (!success) {
                            statusMessage = "❌ Error: Failed to create directory '" + inputBuffer + "'";
                        } else {
                            statusMessage = "✅ Directory '" + inputBuffer + "' created successfully.";
                        }
                    }
                    mode = BROWSE;
                } else if (mode == INPUT_FILE) {
                    if (!inputBuffer.empty()) {
                        bool success = fs.touch(inputBuffer);
                        if (!success) {
                            statusMessage = "❌ Error: Failed to create file '" + inputBuffer + "'";
                        } else {
                            statusMessage = "✅ File '" + inputBuffer + "' created successfully.";
                        }
                    }
                    mode = BROWSE;
                } else if (mode == SEARCH_INPUT) {
                    if (!inputBuffer.empty()) {
                        searchQuery = inputBuffer;
                        searchResults = fs.searchNodes(searchQuery);
                        if (searchResults.empty()) {
                            statusMessage = "🔍 No matches found for '" + searchQuery + "'.";
                            mode = BROWSE;
                        } else {
                            mode = SEARCH_RESULTS;
                            selectedIndex = 0;
                            scrollOffset = 0;
                        }
                    } else {
                        mode = BROWSE;
                    }
                }
                inputBuffer = "";
            } else if (key == KEY_ESC) {
                inputBuffer = "";
                mode = BROWSE;
            } else if (key == KEY_BACKSPACE) {
                if (!inputBuffer.empty()) {
                    inputBuffer.pop_back();
                }
                originalSearchPrefix = inputBuffer;
                autocompleteCycleIndex = -1;
            } else if (key == KEY_TAB && mode == SEARCH_INPUT) {
                std::vector<std::string> matches = originalSearchPrefix.empty() ? std::vector<std::string>{} : fs.getIndex().getKeysWithPrefix(originalSearchPrefix);
                if (!matches.empty()) {
                    if (autocompleteCycleIndex == -1) {
                        std::string lcp = findLongestCommonPrefix(matches);
                        if (lcp.length() > originalSearchPrefix.length()) {
                            inputBuffer = lcp;
                        } else {
                            autocompleteCycleIndex = 0;
                            inputBuffer = matches[0];
                        }
                    } else {
                        autocompleteCycleIndex = (autocompleteCycleIndex + 1) % matches.size();
                        inputBuffer = matches[autocompleteCycleIndex];
                    }
                }
            } else if (key >= 32 && key <= 126) {
                inputBuffer.push_back((char)key);
                originalSearchPrefix = inputBuffer;
                autocompleteCycleIndex = -1;
            }
        } else if (mode == SEARCH_RESULTS) {
            if (key == KEY_UP) {
                selectedIndex--;
            } else if (key == KEY_DOWN) {
                selectedIndex++;
            } else if (key == KEY_ESC) {
                mode = BROWSE;
                selectedIndex = 0;
                scrollOffset = 0;
            } else if (key == KEY_ENTER) {
                if (!items.empty() && selectedIndex < (int)items.size()) {
                    FileNode* match = items[selectedIndex].node;
                    if (match != nullptr) {
                        FileNode* targetDir = (match->type == NodeType::DIRECTORY) ? match : match->parent;
                        if (targetDir != nullptr) {
                            fs.cd(fs.getAbsolutePath(targetDir));
                            mode = BROWSE;
                            
                            items.clear();
                            currentDir = fs.getCurrentDir();
                            if (currentDir != rootNode) {
                                items.push_back({"..", true, nullptr});
                            }
                            std::vector<FileNode*> children = fs.getCurrentDirChildren();
                            for (FileNode* childNode : children) {
                                items.push_back({childNode->name, childNode->type == NodeType::DIRECTORY, childNode});
                            }
                            
                            selectedIndex = 0;
                            for (size_t i = 0; i < items.size(); ++i) {
                                if (items[i].node == match) {
                                    selectedIndex = i;
                                    break;
                                }
                            }
                            scrollOffset = 0;
                        }
                    }
                }
            }
        } else {
            if (key == KEY_UP) {
                selectedIndex--;
            } else if (key == KEY_DOWN) {
                selectedIndex++;
            } else if (key == KEY_LEFT || key == KEY_BACKSPACE) {
                if (currentDir != rootNode) {
                    std::string currentDirName = currentDir->name;
                    fs.cd("..");
                    
                    items.clear();
                    currentDir = fs.getCurrentDir();
                    if (currentDir != rootNode) {
                        items.push_back({"..", true, nullptr});
                    }
                    std::vector<FileNode*> children = fs.getCurrentDirChildren();
                    for (FileNode* childNode : children) {
                        items.push_back({childNode->name, childNode->type == NodeType::DIRECTORY, childNode});
                    }
                    
                    selectedIndex = 0;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (items[i].node != nullptr && items[i].node->name == currentDirName) {
                            selectedIndex = i;
                            break;
                        }
                    }
                    scrollOffset = 0;
                }
            } else if (key == KEY_RIGHT || key == KEY_ENTER) {
                if (!items.empty() && selectedIndex < (int)items.size()) {
                    TUIItem sel = items[selectedIndex];
                    if (sel.node == nullptr && sel.name == "..") {
                        std::string currentDirName = currentDir->name;
                        fs.cd("..");
                        
                        items.clear();
                        currentDir = fs.getCurrentDir();
                        if (currentDir != rootNode) {
                            items.push_back({"..", true, nullptr});
                        }
                        std::vector<FileNode*> children = fs.getCurrentDirChildren();
                        for (FileNode* childNode : children) {
                            items.push_back({childNode->name, childNode->type == NodeType::DIRECTORY, childNode});
                        }
                        
                        selectedIndex = 0;
                        for (size_t i = 0; i < items.size(); ++i) {
                            if (items[i].node != nullptr && items[i].node->name == currentDirName) {
                                selectedIndex = i;
                                break;
                            }
                        }
                        scrollOffset = 0;
                    } else if (sel.isDir && sel.node != nullptr) {
                        fs.cd(sel.node->name);
                        selectedIndex = 0;
                        scrollOffset = 0;
                    } else if (!sel.isDir) {
                        statusMessage = "📄 Selected file: " + sel.name + " (Enter has no action on files)";
                    }
                }
            } else if (key == 'm') {
                mode = INPUT_DIR;
                inputBuffer = "";
            } else if (key == 't') {
                mode = INPUT_FILE;
                inputBuffer = "";
            } else if (key == 's' || key == '/') {
                mode = SEARCH_INPUT;
                inputBuffer = "";
                originalSearchPrefix = "";
                autocompleteCycleIndex = -1;
            } else if (key == 'v') {
                if (visMode == DIR_TREE) {
                    visMode = LCRS_TREE;
                } else if (visMode == LCRS_TREE) {
                    visMode = BTREE_INDEX;
                } else {
                    visMode = DIR_TREE;
                }
            } else if (key == 'q' || key == KEY_ESC) {
                break;
            }
        }
    }

    rawMode.disable();
    std::cout << "\033[2J\033[H" << "Memory cleaned up. Goodbye!\n";
    return 0;
}
