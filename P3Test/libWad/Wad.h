#include <string>
#include <vector>
#include <map>

class node {
    public:
        bool content;
        // bool map;
        std::string name;
        int size;
        int offset;
        std::vector<std::string> childOrder;
        std::map<std::string, node*> children;

        node(int size = 0, int offset = 0, std::string name = "/", bool content = false);
        node* travel(const std::string &path);
        void addChild(node* child);
        ~node();
};

class Wad {

    node root;
    // char* data;
    std::string ext;
    int fd;
    int numDesc;
    int descOffset;
    std::vector<std::string> descOrder;

    Wad(const std::string &path);

public:
    static Wad* loadWad(const std::string &path);
    std::string getMagic();

    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);

    int getSize(const std::string &path);
    int getContents(const std::string &path, char* buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string>* directory);
    
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char* buffer, int length, int offset = 0);
};

