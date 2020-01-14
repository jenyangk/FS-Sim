#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <bitset>
#include <list>
#include <algorithm>
#include <unistd.h>
#include <iostream>

using namespace std;

/**
 * @brief Tokenize a string 
 * 
 * @param str - The string to tokenize
 * @param delim - The string containing delimiter character(s)
 * @return std::vector<std::string> - The list of tokenized strings. Can be empty
 */
vector<string> tokenize(const string &str, const char *delim)
{
    char *cstr = new char[str.size() + 1];
    strcpy(cstr, str.c_str());

    char *tokenized_string = strtok(cstr, delim);

    vector<string> tokens;
    while (tokenized_string != NULL)
    {
        tokens.push_back(string(tokenized_string));
        tokenized_string = strtok(NULL, delim);
    }
    delete[] cstr;

    return tokens;
}

string back_directory(const string &str)
{
    // Check if str = root
    string dir = str;
    if (dir.compare("root/") == 0)
    {
        return "root/";
    }
    vector<string> dirs = tokenize(dir, "/");

    string path;
    for (uint i = 0; i < dirs.size() - 1; i++)
    {
        path.append(dirs[i].c_str());
        path.append("/");
    }
    return path;
}

map<int, int> contigous_count(char *free_block_list)
{
    // Get all bits
    string free_list;
    for (size_t i = 0; i < 16; i++)
    {
        // First 8 bits of block_list OR with free_block_list[i]
        free_list.append(bitset<8>(free_block_list[i]).to_string());
    }
    reverse(free_list.begin(), free_list.end());
    bitset<128> block_list(free_list);

    // Start counting bits
    map<int, int> contigous_blocks;
    int empty_counter = 0;

    // There are 128 blocks
    for (size_t i = 1; i < 128; i++)
    {
        // Count empty, test() returns true if bit i is set
        if (block_list.test(i))
        {
            if (empty_counter > 0)
            {
                // Reached the a set bit
                contigous_blocks.insert(pair<int, int>(empty_counter, i - empty_counter + 1));
                // Reset empty_counter
                empty_counter = 0;
            }
        }
        else
        {
            if (i == 127)
            {
                if (!block_list.test(127))
                {
                    empty_counter++;
                    // Include last bit is unset
                    contigous_blocks.insert(pair<int, int>(empty_counter, i - empty_counter + 1));
                    // Done here, all blocks reached
                }
            }
            empty_counter++;
        }
    }
    return contigous_blocks;
}

bitset<128> bitset_block_list(char *free_block_list)
{
    bitset<128> block_list(0);

    // Convert bit value of chars to a single string
    for (size_t i = 0; i < 16; i++)
    {
        // 8 bits in a char
        for (size_t j = 0; j < 8; j++)
        {
            if (i == 0)
            {
                i = 1;
            }
            if (bitset<8>(free_block_list[i]).test(j))
            {
                block_list.set(i * j, 1);
            }
        }
    }
    return block_list;
}

void set_block_list(char *free_block_list, int start, int end, bool value)
{
    // Block list in string form
    string block_list_string;
    for (size_t i = 0; i < 16; i++)
    {
        block_list_string.append(bitset<8>(free_block_list[i]).to_string());
    }

    reverse(block_list_string.begin(), block_list_string.end());

    // Set block_list
    bitset<128> block_list(block_list_string);
    for (int i = start; i < end; i++)
    {
        block_list.set(i, value);
    }

    // Build char array
    int index = 0;
    int char_index = 7;
    bitset<8> chars(0);
    for (int i = 0; i < 128; i++)
    {
        chars.set(char_index, block_list.test(i));
        char_index--;
        // Add and reset
        if (char_index < 0)
        {
            free_block_list[index] = (char)(chars.to_ulong());
            chars.reset();
            index++;
            char_index = 7;
        }
    }
}

int empty_block_finder(map<int, int> contigous_blocks, int size)
{
    int starting_block = 0;
    bool found_block;
    map<int, int>::iterator it_map = contigous_blocks.begin();
    // Map is sorted ascending order
    for (; it_map != contigous_blocks.end(); it_map++)
    {
        if (it_map->first >= size)
        {
            starting_block = it_map->second;
            found_block = true;
            break;
        }
    }

    // No block found
    if (!found_block)
    {
        return -1;
    }
    return starting_block;
}

void updateBlock(int FD, char *buffer, int offset)
{
    if (pwrite(FD, buffer, 1024, offset) < 0)
    {
        cerr << "Error: Cannot write to block." << endl;
    }
}

void moveDB(int FD, int start, int end, int newStart, int newEnd)
{
    int BLOCK_SIZE = 1024;
    char buffer[1024];
    char emptyBuffer[1024];

    memset(emptyBuffer, 0, 1024);

    for (int i = start; i < end; i++)
    {
        // Copy out
        if (pread(FD, buffer, BLOCK_SIZE, start * BLOCK_SIZE) < 0)
        {
            cerr << "Error: Cannot read from block." << endl;
        }
        // Copy in
        if (pwrite(FD, buffer, BLOCK_SIZE, newStart * BLOCK_SIZE) < 0)
        {
            cerr << "Error: Cannot write to block." << endl;
        }
        // Zero out old data block
        // Copy out
        if (pwrite(FD, emptyBuffer, BLOCK_SIZE, start * BLOCK_SIZE) < 0)
        {
            cerr << "Error: Cannot read from block." << endl;
        }
        newStart++;
    }
}
