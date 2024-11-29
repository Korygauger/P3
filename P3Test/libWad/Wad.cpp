#include "Wad.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <regex>
#include <stack>

Wad* Wad::loadWad(const std::string &path){
    Wad* wad = new Wad(path);
// regix?
    return wad;
}

Wad::Wad(const std::string &path) {
    int fd = open(path.c_str(), O_RDWR);
    if (fd == -1) {
        std::cout << "Cannont open wad file!" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->fd = fd;
    this->root = node(0,0,"/");
    node* currNode = &root;

    char magic[4];
    ssize_t m = pread(fd, &magic, 4, 0);
    if (m == -1){
        std::cout << "Can't read magic" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (magic[0] == 'I')
        ext = "IWAD";
    else
        ext = "PWAD";


    int b = pread(fd, &numDesc, 4, 4);
    if (b == -1) {
        std::cout << "unable to read header" << std::endl;
        exit(EXIT_FAILURE);
    }

    int b2 = pread(fd, &descOffset, 4, 8);
    if (b2 == -1) {
        std::cout << "unable to read header" << std::endl;
        exit(EXIT_FAILURE);
    }

    int offset = 0;
    int size = 0;
    std::string name = "";
    char cname[8];

    int currOffset = descOffset;
    std::stack<node*> parents;

    int mapCount = 0;
    bool inMap = false;


    for (int i = 0; i < numDesc; i++) {
        int b1 = pread(fd, &offset, 4, currOffset);
        currOffset+=4;
        int b2 = pread(fd, &size, 4, currOffset);
        currOffset+=4;
        int b3 = pread(fd, &cname, 8, currOffset);
        currOffset+=8;

        name = cname;
        // if (name != "")
        //     std::cout << name << ", " << offset << ", " << size << std::endl;

        descOrder.push_back(name);

        if (std::regex_match(name, std::regex("(.*)(_START)"))) {
            std::regex start("(_START)");
            std::string nm = std::regex_replace(name, start, "");
            node* temp = new node(size, offset, nm, false);
            currNode->addChild(temp);
            parents.push(currNode);
            currNode = temp;
            continue;
        }

        else if (std::regex_match(name, std::regex("(.*)(_END)"))) {
            currNode = parents.top();
            parents.pop();
            continue;
        }

        else if (std::regex_match(name, std::regex("(E)(\\d)(M)(\\d)"))) {
            inMap = true;
            node* temp = new node(size, offset, name, false);
            currNode->addChild(temp);
            parents.push(currNode);
            currNode = temp;
            continue;
        }

        node* temp = new node(size, offset, name, true);
        currNode->addChild(temp);
        
        if (inMap) {
            mapCount++;
            if (mapCount == 10) {
                currNode = parents.top();
                parents.pop();
                inMap = false;
                mapCount = 0;
            }
        }
            
    }
}

std::string Wad::getMagic(){
    return this->ext;
}

bool Wad::isContent(const std::string &path){
    node* n = root.travel(path);
    if (n == nullptr) return false; 
    return n->content;
}
bool Wad::isDirectory(const std::string &path){
    node* n = root.travel(path);
    if (n == nullptr) return false; 
    return !n->content;
}

int Wad::getSize(const std::string &path){
    node* n = root.travel(path);
    if (n == nullptr || !n->content) return -1; 
    return n->size;
}

int Wad::getContents(const std::string &path, char* buffer, int length, int offset){
    node* n = root.travel(path);
    if (n == nullptr) {
        std::cout << "content is null" << std::endl;
        return -1;
    }
    if (!n->content) {
        std::cout << "isn't content: getContents" << std::endl;
        return -1;
    }
    else if (n->size == 0) {
        std::cout << "no content" << std::endl;
        return 0;
    }
    else if (offset > n->size) {
        std::cout << "Offset too large" << std::endl;
        return 0;
    }

    int acOff = n->offset + offset;

    if (length > n->size - offset) {
        length = n->size - offset;
    }

    ssize_t b = pread(this->fd, buffer, length, acOff);
    if (b == -1)
        return -1;

    return b;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string>* directory){
    node* n = root.travel(path);
    if (n == nullptr)
        return -1;
    else if (n->content)
        return -1;

    for (int i = 0; i < n->children.size(); i++) {
        directory->push_back(n->childOrder[i]);
    }

    return n->children.size();
}

void Wad::createDirectory(const std::string &path){

    std::stringstream spath(path);
    std::string token;
    std::vector<std::string> vPath;
    while (getline(spath, token, '/')) {
        if (token == "") continue;
        vPath.push_back(token);
    }

    std::string parentDir = "/";
    for (int i = 0; i < vPath.size() - 1; i++) {
        parentDir += vPath[i] + "/";
    }
    std::string dirName = vPath[vPath.size()-1];

    if (dirName.size() > 2) {
        std::cout << dirName << " is too long for a directory name" << std::endl;
        return;
    }

    std::string dirStart = dirName + "_START";
    std::string dirEnd = dirName + "_END";


    node* parentNode = root.travel(parentDir);
    if (parentNode == nullptr) {
        std::cout << "invalid parent directory" << std::endl;
        return;
    }

    node* newDir = new node(0,0,dirName,false);
    parentNode->addChild(newDir);
    int moveOffset;
    if (parentNode->name == "/") {
        moveOffset = descOffset + (numDesc*16);
        descOrder.push_back(dirStart);
        descOrder.push_back(dirEnd);
    }
    else {
        std::string moveName = "";
        moveName += parentNode->name + "_END";

        bool inParent = false;
        for (int i = 0; i < vPath.size()-1; i++) {
            vPath[i] += "_START";
        }
        vPath[vPath.size()-1] = moveName;

        int aboveParent = 0;
        int moveIndex = 0;
        bool root = true;
        int pathIndex = 0;
        for (int i = 0; i < descOrder.size(); i++) {
            if (inParent) {
                if (descOrder[i] == vPath[pathIndex] && aboveParent == 0) {
                    moveIndex = i;
                    descOrder.insert(descOrder.begin() + i,dirEnd);
                    descOrder.insert(descOrder.begin() + i,dirStart);
                    break;
                }
                else if (std::regex_match(descOrder[i], std::regex("(.*)(_START)"))) 
                    aboveParent++;
                else if (std::regex_match(descOrder[i], std::regex("(.*)(_END)")))
                    aboveParent--;

            }
            else if (descOrder[i] == vPath[pathIndex]) {
                pathIndex++;
                if (pathIndex == vPath.size()-1)
                    inParent = true;
            }
        }

        int moveSize = (numDesc - moveIndex) * 16;
        moveOffset = descOffset + (moveIndex * 16);

        char* moveBytes = new char[moveSize];

        int b = pread(fd, moveBytes, moveSize, moveOffset);
        if (b == -1) {
            std::cout << "unable to read moveBytes: createDir" << std::endl;
            return;
        }

        int b2 = pwrite(fd, moveBytes, moveSize, moveOffset + 32);
        if (b2 == -1) {
            std::cout << "unable to write moveBytes: createDir" << std::endl;
            return;
        }

        delete [] moveBytes;
    }

    int os = 0;
    int b6 = pwrite(fd, &os, 4, moveOffset);
    if (b6 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }
    int b7 = pwrite(fd, &os, 4, moveOffset + 4);
    if (b7 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }

    int b3 = pwrite(fd, dirStart.data(), dirStart.size(), moveOffset + 8);
    if (b3 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }

    int b9 = pwrite(fd, &os, 4, moveOffset + 16);
    if (b9 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }
    int b10 = pwrite(fd, &os, 4, moveOffset + 16 + 4);
    if (b10 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }
    dirEnd += '\0';
    int b5 = pwrite(fd, dirEnd.data(), dirEnd.size(), moveOffset + 16 + 8);
    if (b5 == -1) {
        std::cout << "unable to write newDescriptors: createDir" << std::endl;
        return;
    }
    

    numDesc += 2;
    
    int b4 = pwrite(fd, &numDesc, 4, 4);
    if (b4 == -1) {
        std::cout << "unable to write numDesc: createDir" << std::endl;
        return;
    }
}

void Wad::createFile(const std::string &path){

    std::stringstream spath(path);
    std::string token;
    std::vector<std::string> vPath;
    while (getline(spath, token, '/')) {
        if (token == "") continue;
        vPath.push_back(token);
    }

    std::string parentDir = "/";
    for (int i = 0; i < vPath.size() - 1; i++) {
        parentDir += vPath[i] + "/";
    }
    std::string fileName = vPath[vPath.size()-1];

    if (std::regex_match(fileName, std::regex("(.*)(_START)")) || 
            std::regex_match(fileName, std::regex("(.*)(_END)")) || 
            std::regex_match(fileName, std::regex("(E)(\\d)(M)(\\d)"))) {
        std::cout << "filename contains an illegal string" << std::endl;
        return;
    }
    else if (fileName.size() > 8) {
        std::cout << "filename too big!" << std::endl;
        return;
    }

    node* parentNode = root.travel(parentDir);
    if (parentNode == nullptr) {
        std::cout << "invalid parent directory" << std::endl;
        return;
    }
    if (std::regex_match(parentNode->name, std::regex("(E)(\\d)(M)(\\d)"))) {
        std::cout << "cannot add files to maps" << std::endl;
        return;
    }

    node* newFile = new node(0,0,fileName,true);
    parentNode->addChild(newFile);
    int moveOffset;
    if (parentNode->name == "/") {
        moveOffset = descOffset + (numDesc*16);
        descOrder.push_back(fileName);
    }
    else {
        std::string moveName = "";
        moveName += parentNode->name + "_END";

        bool inParent = false;
        for (int i = 0; i < vPath.size()-1; i++) {
            vPath[i] += "_START";
        }
        vPath[vPath.size()-1] = moveName;

        int aboveParent = 0;
        int moveIndex = 0;
        bool root = true;
        int pathIndex = 0;
        for (int i = 0; i < descOrder.size(); i++) {
            if (inParent) {
                if (descOrder[i] == vPath[pathIndex] && aboveParent == 0) {
                    moveIndex = i;
                    descOrder.insert(descOrder.begin() + i,fileName);
                    break;
                }
                else if (std::regex_match(descOrder[i], std::regex("(.*)(_START)"))) 
                    aboveParent++;
                else if (std::regex_match(descOrder[i], std::regex("(.*)(_END)")))
                    aboveParent--;

            }
            else if (descOrder[i] == vPath[pathIndex]) {
                pathIndex++;
                if (pathIndex == vPath.size()-1)
                    inParent = true;
            }
        }

        int moveSize = (numDesc - moveIndex) * 16;
        moveOffset = descOffset + (moveIndex * 16);

        uint8_t* moveBytes = new uint8_t[moveSize];

        int b = pread(fd, moveBytes, moveSize, moveOffset);
        if (b == -1) {
            std::cout << "unable to read moveBytes: createFile" << std::endl;
            return;
        }

        int b2 = pwrite(fd, moveBytes, moveSize, moveOffset + 16);
        if (b2 == -1) {
            std::cout << "unable to write moveBytes: createFile" << std::endl;
            return;
        }

        delete [] moveBytes;
    }
    int so = 0;
    int b3 = pwrite(fd, &so, 4, moveOffset); 
    int b4 = pwrite(fd, &so, 4, moveOffset + 4); 

    // if (fileName.size() < 7) 
    //     fileName += '\0';
    
    int b5 = pwrite(fd, fileName.data(), fileName.size(), moveOffset + 8);

    numDesc++;
    
    int b6 = pwrite(fd, &numDesc, 4, 4);
    if (b6 == -1) {
        std::cout << "unable to write numDesc: createFile" << std::endl;
        return;
    }


}

int Wad::writeToFile(const std::string &path, const char* buffer, int length, int offset){

    // std::cout << path << std::endl;

    node* n = root.travel(path);
    if (n == nullptr)
        return -1;
    else if (n->size > 0)
        return 0;

    if (std::regex_match(n->name, std::regex("(E)(\\d)(M)(\\d)"))) {
        std::cout << "cannot write to maps" << std::endl;
        return -1;
    }
    int moveOffset = descOffset;
    int moveSize = (numDesc *16);
    if (offset > 0) {
        moveSize += descOffset - offset;
        moveOffset = offset;
    }

    uint8_t* moveBytes = new uint8_t[moveSize];
    int b = pread(fd, moveBytes, moveSize, moveOffset);
    if (b == -1) {
        std::cout << "unable to read moveBytes: write to File" << std::endl;
        return 0;
    }

    int b2 = pwrite(fd, moveBytes, moveSize, moveOffset + length);
    if (b2 == -1) {
        std::cout << "unable to write moveBytes: write to file" << std::endl;
        return 0;
    }

    int b3 = pwrite(fd, buffer, length, moveOffset);
    if (b3 == -1) {
        std::cout << "unable to write buffer: write to file" << std::endl;
        return 0;
    }

    n->offset = moveOffset;
    n->size = length;
    descOffset = moveOffset + length;

    int b4 = pwrite(fd, &descOffset, 4, 8); 
    if (b4 == -1) {
        std::cout << "unable to write descOffset: write to file" << std::endl;
        return 0;
    }

    std::stringstream spath(path);
    std::string token;
    std::vector<std::string> vPath;
    while (getline(spath, token, '/')) {
        if (token == "") continue;
        else if (token != n->name)
            token += "_START";
        vPath.push_back(token);
    }

    bool inParent = false;
    if (vPath.size() == 1) {
        inParent = true;
    }

    int aboveParent = 0;
    int descIndex = 0;
    bool root = true;
    int pathIndex = 0;
    for (int i = 0; i < descOrder.size(); i++) {
        if (inParent) {
            if (descOrder[i] == n->name && aboveParent == 0) {
                descIndex = i;
                break;
            }
            else if (std::regex_match(descOrder[i], std::regex("(.*)(_START)"))) 
                aboveParent++;
            else if (std::regex_match(descOrder[i], std::regex("(.*)(_END)")))
                aboveParent--;
            
        }
        else if (descOrder[i] == vPath[pathIndex]) {
            pathIndex++;
            if (pathIndex == vPath.size()-1)
                inParent = true;
        }
    }

    // if (std::regex_match(fileName, std::regex("(.*)(_START)")) || 
    //         std::regex_match(fileName, std::regex("(.*)(_END)")) || 
    //         std::regex_match(fileName, std::regex("(E)(\\d)(M)(\\d)"))) {
    int nDescOffset = descOffset + (descIndex * 16);

    int b5 = pwrite(fd, &n->offset, 4, nDescOffset);
    int b6 = pwrite(fd, &n->size, 4, nDescOffset + 4);

    

    return b3;
}

node::node(int size, int offset, std::string name, bool content){
    this->size = size;
    this->offset = offset;
    this->name = name;
    this->content = content;
}

node* node::travel(const std::string &path) {

    node* returnNode = nullptr;

    if (path == "/") {
        return this;
    }
    else if (path == "") {
        return nullptr;
    }
    
    std::vector<std::string> vPath;
    std::stringstream spath(path);
    std::string token;
    while (getline(spath, token, '/')) {
        if (token == "") continue;
        vPath.push_back(token);
    }

    if (children.find(vPath[0]) != children.end()) {
        returnNode = children[vPath[0]];
    }
    else {
        std::cout << "could not find node: " << vPath[0] << " in root node" << std::endl;
        return nullptr;
    }


    for (int i = 1; i < vPath.size(); i++) {
        if (returnNode->children.find(vPath[i]) != returnNode->children.end()) {
            returnNode = returnNode->children[vPath[i]];
        }
        else {
            std::cout << "Could not find node: " << vPath[i] << " in node: " << returnNode->name << std::endl;
            return nullptr;
        }
    }

    return returnNode;
}

void node::addChild(node* child) {
    // std::cout << "Parent: " << this->name << " Child: " << child->name << std::endl;
    this->childOrder.push_back(child->name);
    children[child->name] = child;
}

node::~node() {
    for (auto itr = children.begin(); itr != children.end(); itr++) {
        delete itr->second;
    }
}
