compile
gcc -Wall fuse_study.c ../custom_include/bound.c `pkg-config fuse3 --cflags --libs` -o fs_cli