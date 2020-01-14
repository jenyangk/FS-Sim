#include <vector>
#include <map>
#include <string>
#include <bitset>

// String tokenizer
std::vector<std::string> tokenize(const std::string &str, const char *delim);

// Returns a string of the parent directory of the directory in string form
std::string back_directory(const std::string &str);

// map<total size, start_block>
std::map<int, int> contigous_count(char* free_block_list);


// Convert free_block_list to 128 bits
std::bitset<128> bitset_block_list(char* free_block_list);

// Sets block[start, end] to the bool value in the free_block_list
void set_block_list(char* free_block_list, int start, int end, bool value);

// Returns a start block that is contigously empty for at least size
int empty_block_finder(std::map<int, int> contigous_blocks, int size);

// Write buffer into disk at offset
void updateBlock(int FD, char *buffer, int offset);

// Move datablocks from [start, end] to [newStart, newEnd]
void moveDB(int FD, int start, int end, int newStart, int newEnd);