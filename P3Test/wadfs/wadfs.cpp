#define FUSE_USE_VERSION 26
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fuse.h>
#include "../libWad/Wad.h"


static int getattr_callback(const char* path, struct stat* st) {
    Wad* myWad = (Wad*)fuse_get_context()->private_data;
    if (myWad->isDirectory(path)) {
        st->st_mode = S_IFDIR | 0777;
        // st->st_nlink = 2;
        return 0;
    }

    else if (myWad->isContent(path)) {
        st->st_mode = S_IFREG | 0777;
        // st->st_nlink = 1;
        st->st_size = myWad->getSize(path);
        return 0;
    }

    return -1;
    
}

static int mknod_callback(const char* path, mode_t mode, dev_t rdev) {
    Wad* myWad = (Wad*)fuse_get_context()->private_data;
    myWad->createFile(path);
    return 0;

}

static int mkdir_callback(const char* path, mode_t mode) {
    std::cout << "made it here" << std::endl;
    Wad* myWad = (Wad*)fuse_get_context()->private_data;
    myWad->createDirectory(path);
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* myWad = (Wad*)fuse_get_context()->private_data;

    if (myWad->isContent(path)) {
        return myWad->getContents(path, buf, size, offset);
    }

    std::cout << "something went wrong in read" << std::endl;
    return -1;
}

static int write_callback(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    Wad* myWad = (Wad*)fuse_get_context()->private_data;
    if (myWad->isContent(path)) {
        return myWad->writeToFile(path, buf, size, offset);
    }

    std::cout << "something went wrong in write" << std::endl;
    return -1;
    
}

static int readdir_callback(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    Wad* myWad = (Wad*)fuse_get_context()->private_data;

    std::vector<std::string> directory;
    myWad->getDirectory(path, &directory);

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    for (int i = 0; i < directory.size(); i++) {
        filler(buf, directory[i].c_str(), NULL, 0);
    }

    return 0;
}

// get_attr, mknod, mkdir, read, write, readdir
static struct fuse_operations operations = {
    .getattr = getattr_callback,
    .mknod = mknod_callback,
    .mkdir = mkdir_callback,
    .read = read_callback,
    .write = write_callback,
    .readdir = readdir_callback,
};

// main from Ernesto's video
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout  << "Not enough arguments." << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc-2];

    if(wadPath.at(0) != '/') {
        wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc-2] = argv[argc-1];

    argc--;

    return fuse_main(argc, argv, &operations, myWad);
}
