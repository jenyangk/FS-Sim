#include <map>
#include <string>
#include <vector>

#include "FileSystem.h"

// Consistency Checker
int ccheck(Super_block *super_block);

/**
 * Blocks that are marked free in the free-space list cannot be 
 * allocated to any file. Similarly, blocks marked in use in the 
 * free-space list must be allocated to exactly one file.
 */ 
int check1(Super_block *super_block);

/**
 * The name of every file/directory must be unique in each directory.
 */ 
int check2(Super_block *super_block);

/**
 * If the state of an inode is free, all bits in this inode must be zero. 
 * Otherwise, the name attribute stored in the inode must have at least one bit that is not zero.
 */
int check3(Super_block *super_block);

/**
 * The start block of every inode that is marked as a file 
 * must have a value between 1 and 127 inclusive.
 */ 
int check4(Super_block *super_block);


/**
 * The size and start block of an inode that is 
 * marked as a directory must be zero.
 */ 
int check5(Super_block *super_block);

/**
 * For every inode, the index of its parent inode cannot be 126. 
 * Moreover, if the index of the parent inode is between 0 and 125 
 * inclusive, then the parent inode must be in use and marked 
 * as a directory.
 */ 
int check6(Super_block *super_block);

/**
 * Super_block deserializer 
 */ 
void deserializeSB(char *block, Super_block *super_block);

/**
 * Creates a tree for all the directories and files in the Super_block
 */ 
std::map<std::string, std::vector<int>> buildFS(Super_block *super_block);

/**
 * Adds child inodes to a vector.
 */ 
void childInodes(Super_block *super_block, std::vector<int>& inodes, int parentInode);