output: wadfs

wadfs: ../libWad/libWad.a wadfs.cpp
	g++ -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 wadfs.cpp -o wadfs -lfuse -L ../libWad -lWad 
