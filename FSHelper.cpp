#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <unistd.h>

#include <bitset>

#include <set>
#include <map>
#include <vector>
#include <list>

#include "FSHelper.h"

#include <iostream>

using namespace std;

int check1(Super_block *super_block)
{
    // Represent the character array in binary form
    bitset<128> inode_used_list(0);

    // Check all Inode
    for (size_t i = 0; i < 126; i++)
    {
        // Set bits between start_block til start_block + file_size
        // Get start_block
        int start = super_block->inode[i].start_block;

        if (start <= 127)
        {
            int used_size = bitset<7>(super_block->inode[i].used_size).to_ulong();
            int end = start + used_size;

            for (int j = start; j < end; j++)
            {
                // Set j-th bit to 1, meaning occupied
                // Check if it is already set, possible overlap
                if (inode_used_list.test(j))
                {
                    // Overlap!
                    return 0;
                }
                inode_used_list.set(j, 1);
            }
        }
    }

    // First bit must be one, for superblock
    inode_used_list.set(0, 1);

    string free_block_list;
    // Convert bit value of chars to a single string
    for (size_t i = 0; i < 16; i++)
    {
        free_block_list.append(bitset<8>(super_block->free_block_list[i]).to_string());
    }

    reverse(free_block_list.begin(), free_block_list.end());

    // If matches
    if (free_block_list.compare(inode_used_list.to_string()) == 0)
    {
        return 1;
    }

    // Else
    return 0;
}

// Check 2
// Fail: 0, Success: 1
int check2(Super_block *super_block)
{
    // inode names must be unique within their respective directory
    // map<file_name, inodeID>
    map<string, vector<int>> file_names;

    // Check all 126 Inodes
    for (int i = 0; i < 126; i++)
    {
        // Test last bit of used_size
        // If last bit = 1, inode in use
        if (bitset<8>(super_block->inode[i].used_size).test(7))
        {
            string name(super_block->inode[i].name, 5);
            // Check if name exists
            if (file_names.count(name) > 0)
            {
                vector<int>::iterator it = file_names[name.c_str()].begin();
                for (; it != file_names[name.c_str()].end(); it++)
                {
                    // Check if dir_parent matches another
                    if (i != *it && bitset<7>(super_block->inode[i].dir_parent) == bitset<7>(super_block->inode[*it].dir_parent))
                    {
                        return 0;
                    }
                    file_names[name].push_back(i);
                }
            }
            else
            {
                file_names.insert(pair<string, vector<int>>(name, {i}));
            }
        }
    }
    return 1;
}

// Check 3
// Fail: 0, Success: 1
int check3(Super_block *super_block)
{
    // Check Inode's state, if free (0) all bits must be 0
    // Check all 126 Inodes
    for (size_t i = 0; i < 126; i++)
    {
        // Test last bit of used_size, inode's state
        // if not free, check name exists
        if (bitset<8>(super_block->inode[i].used_size).test(7))
        {
            // Name attribute must have at least one bit
            string nameCheck = super_block->inode[i].name;
            if (nameCheck.empty())
            {
                return 0;
            }
        }
        else
        {
            // Name check
            for (size_t j = 0; j < 5; j++)
            {
                if (bitset<8>(super_block->inode[i].name[j]).any())
                {
                    return 0;
                }
            }

            // if free, everything in inode must be 0
            if (super_block->inode[i].used_size == 0 && super_block->inode[i].dir_parent == 0 && super_block->inode[i].start_block == 0)
            {
                continue;
            }
            return 0;
        }
    }
    return 1;
}

// Check 4
// Fail: 0, Success: 1
int check4(Super_block *super_block)
{
    // Check Inode's start block, must be 1 to 127 (inclusive)

    // Check all 126 Inodes
    for (size_t i = 0; i < 126; i++)
    {
        // Check inode's state (used_size), 1 in use
        // Check inode's type (dir_parent), 0 for file
        if (bitset<8>(super_block->inode[i].used_size).test(7) && !bitset<8>(super_block->inode[i].dir_parent).test(7))
        {
            // if inode's start_block <= 0 or >= 128, return 0 for fail
            if (super_block->inode[i].start_block <= 0 || super_block->inode[i].start_block >= 128)
            {
                return 0;
            }
        }
    }
    return 1;
}

// Check 5
// Fail: 0, Success: 1
int check5(Super_block *super_block)
{
    // Check Inode's size and start_block, must be 0 for directory

    // Check all 126 Inodes
    for (size_t i = 0; i < 126; i++)
    {
        // Check inode's state (used_size), 1 in use
        // Check Inode's type, 1 for directory
        if (bitset<8>(super_block->inode[i].used_size).test(7) && bitset<8>(super_block->inode[i].dir_parent).test(7))
        {
            // Inode's size == 1000 0000 and start_block = 0
            if (super_block->inode[i].used_size != 128 || super_block->inode[i].start_block != 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

// Check 6
// Fail: 0, Success: 1
int check6(Super_block *super_block)
{
    // Check all 126 Inodes
    for (size_t i = 0; i < 126; i++)
    {
        if (bitset<8>(super_block->inode[i].used_size).test(7))
        {

            // Parent directory cannot be 126
            if (super_block->inode[i].dir_parent == 126)
            {
                return 0;
            }

            int parentInode = bitset<7>(super_block->inode[i].dir_parent).to_ulong();
            // Check last bit of dir_parent
            // Ignore root
            if (parentInode != 127)
            {
                if (!bitset<8>(super_block->inode[parentInode].used_size).test(7) || !bitset<8>(super_block->inode[parentInode].dir_parent).test(7))
                {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int ccheck(Super_block *super_block)
{
    // Consistency Checking
    // Returns smallest error code
    if (check1(super_block) == 0)
    {
        return 1;
    }

    if (check2(super_block) == 0)
    {
        return 2;
    }

    if (check3(super_block) == 0)
    {
        return 3;
    }

    if (check4(super_block) == 0)
    {
        return 4;
    }

    if (check5(super_block) == 0)
    {
        return 5;
    }

    if (check6(super_block) == 0)
    {
        return 6;
    }

    // Success
    return 0;
}

void deserializeSB(char *block, Super_block *super_block)
{
    char *b = (char *)block;

    // deserialize free_block_list
    for (int i = 0; i < 16; i++)
    {
        super_block->free_block_list[i] = *b;
        b++;
    }

    // deserialize 126 Inodes
    for (int i = 0; i < 126; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            super_block->inode[i].name[j] = *b;
            b++;
        }
        super_block->inode[i].used_size = (uint8_t)*b;
        b++;
        super_block->inode[i].start_block = (uint8_t)*b;
        b++;
        super_block->inode[i].dir_parent = (uint8_t)*b;
        b++;
    }
}

map<string, vector<int>> buildFS(Super_block *super_block)
{
    string root_dir = "root/";
    map<string, vector<int>> tree;

    // init tree
    tree.insert(pair<string, vector<int>>("root/", {}));

    for (int i = 0; i < 126; i++)
    {
        // Check Inode's state ([0]000 0000)
        if (bitset<8>(super_block->inode[i].used_size).test(7))
        {
            // Build file/directory's directory
            // Get parent directories until root
            // LIFO
            list<string> path;
            int current_inode = i;
            for (;;)
            {
                if (bitset<7>(super_block->inode[current_inode].dir_parent).all())
                {
                    path.push_back("root/");
                    break;
                }
                path.push_back(super_block->inode[current_inode].name);
                current_inode = (int)bitset<7>(super_block->inode[i].dir_parent).to_ulong();
            }

            string directory;
            while (!path.empty())
            {
                directory += path.front();
                path.pop_front();
            }

            // Check if key exists
            int check = tree.count(directory);
            if (check > 0)
            {
                tree[directory].push_back(i);
            }
            else
            {
                // Add new directory key to tree and Inode ID
                tree.insert(pair<string, vector<int>>(directory, {i}));
            }
        }
    }
    return tree;
}

void childInodes(Super_block *super_block, vector<int> &inodes, int parentInode)
{
    // Check all inodes
    for (int i = 0; i < 126; i++)
    {
        // Check if inode has the right parent inode
        if (bitset<7>(super_block->inode[i].dir_parent).to_ulong() == (ulong) parentInode)
        {
            // Check if the that inode has childs
            if (bitset<8>(super_block->inode[i].dir_parent).test(7))
            {
                childInodes(super_block, inodes, i);
            }
            inodes.push_back(i);
        }
    }
}