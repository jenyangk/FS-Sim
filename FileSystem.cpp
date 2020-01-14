#include <cstdio>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <string>
#include <cstring>
#include <bitset>
#include <map>
#include <list>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "FSHelper.h"
#include "Helper.h"

using namespace std;

// Global Variables
int FILE_DESCRIPTOR = -1;     // Mounted disk
bitset<8> CURR_DIRECTORY;     // Current working directory
string CURR_DIRECTORY_STRING; // Current working directory in string form
string DISK_NAME;             // Mounted disk name
bool MOUNTED;                 // Mounted checker

Super_block *SUPER_BLOCK = nullptr; // Super_block
char BUFFER[1024];                  // Buffer

map<string, vector<int>> FILE_TREE; // Pointer to a file tree

ssize_t BLOCK_SIZE = 1024; // BLOCK_SIZE

// Return inode of name in the current directory
int inodeSearch(char name[5])
{
    // Check if file is under the working directory
    vector<int>::iterator it_map = FILE_TREE[CURR_DIRECTORY_STRING].begin();
    while (it_map != FILE_TREE[CURR_DIRECTORY_STRING].end())
    {
        if (strncmp(SUPER_BLOCK->inode[*it_map].name, name, 5) == 0)
        {
            return *it_map;
        }
        it_map++;
    }
    return -1;
}

// Update superblock onto disk
void updateSB()
{
    char buffer[1024];
    strncpy(buffer, SUPER_BLOCK->free_block_list, 16);
    int bufferIndex = 16;
    // 126 Inodes
    for (int i = 0; i < 126; i++)
    {
        // 5 characters for inode's name
        for (int j = 0; j < 5; j++)
        {
            buffer[bufferIndex] = SUPER_BLOCK->inode[i].name[j];
            bufferIndex++;
        }
        // used_size
        buffer[bufferIndex] = (char)SUPER_BLOCK->inode[i].used_size;
        bufferIndex++;
        // start_block
        buffer[bufferIndex] = (char)SUPER_BLOCK->inode[i].start_block;
        bufferIndex++;
        // dir_parent
        buffer[bufferIndex] = (char)SUPER_BLOCK->inode[i].dir_parent;
        bufferIndex++;
    }
    // Update the superblock
    updateBlock(FILE_DESCRIPTOR, buffer, 0);
}

void fs_mount(char *new_disk_name)
{
    // Make sure there is a disk name
    assert(new_disk_name != NULL);

    // 1KB Buffer
    char buffer[1024];

    // Bytes
    ssize_t block;

    // Open disk
    int FD = open(new_disk_name, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (FD < 0)
    {
        cerr << "Error: Cannot find disk " << new_disk_name << "." << endl;
        return;
    }

    // First 1KB is SuperBlock
    // Get first block, read 1KB from disk
    block = pread(FD, buffer, BLOCK_SIZE, 0);
    if (block < 0)
    {
        cerr << "Error: Cannot read block" << endl;
        return;
    }

    Super_block *super_block = new Super_block;
    deserializeSB(buffer, super_block);

    // Consistency Check
    int ccheckVal = ccheck(super_block);
    if (ccheckVal > 0)
    {
        cerr << "Error: File system in " << new_disk_name << " is inconsistent (error code: " << ccheckVal << ")" << endl;

        delete super_block;

        // Mount previous FS
    }
    else
    {
        FILE_DESCRIPTOR = FD;

        if (SUPER_BLOCK != nullptr)
        {
            delete SUPER_BLOCK;
        }

        SUPER_BLOCK = super_block;
        DISK_NAME = new_disk_name;

        // Build File Tree
        // FS Tree
        FILE_TREE = buildFS(SUPER_BLOCK);
        MOUNTED = true;
    }

    // Current work directory should be root/
    // Set all bits to 1, CURR_DIRECTORY = 111 1111
    CURR_DIRECTORY.set();
    CURR_DIRECTORY.set(7, 0);
    CURR_DIRECTORY_STRING = "root/";

    // Mounted!
}

void fs_create(char name[5], int size)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Check if name is illegal
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
    {
        cerr << "File or directory " << name << " already exists" << endl;
        return;
    }

    // Check name in CURR_DIRECTORY_STRING
    int inodeID = inodeSearch(name);
    if (inodeID >= 0)
    {
        // Matched
        cerr << "File or directory ";
        cerr.write(name, 5) << " already exists" << endl;
        return;
    }

    // Find blocks to allocate
    int starting_block = 0;
    if (size > 0)
    {
        // Find contiguous blocks from data blocks
        // We get the most suitable fit for file
        starting_block = empty_block_finder(contigous_count(SUPER_BLOCK->free_block_list), size);
        if (starting_block < 0)
        {
            cerr << "Error: Cannot allocate " << size << " on " << DISK_NAME << endl;
            return;
        }
    }

    // Get empty inode
    inodeID = 0;
    bool found_inode;
    // Check all inodes
    for (size_t i = 0; i < 126; i++)
    {
        if (!bitset<8>(SUPER_BLOCK->inode[i].used_size).test(7))
        {
            inodeID = i;
            found_inode = true;
            break;
        }
    }

    if (!found_inode)
    {
        cerr << "Error: Superblock in disk " << DISK_NAME << " is full, cannot create " << name << endl;
        return;
    }

    // Assign values to inode
    strncpy(SUPER_BLOCK->inode[inodeID].name, name, 5);
    SUPER_BLOCK->inode[inodeID].used_size = (uint8_t)bitset<8>(bitset<8>(size) | bitset<8>("10000000")).to_ulong();
    SUPER_BLOCK->inode[inodeID].start_block = (uint8_t)starting_block;

    bitset<8> dir_parent(0);
    // File: size > 0, Directory: size = 0
    if (size == 0)
    {
        dir_parent.set(7, 1);
    }
    // Get curr_diretory inode
    dir_parent |= CURR_DIRECTORY;

    SUPER_BLOCK->inode[inodeID].dir_parent = (uint8_t)dir_parent.to_ulong();

    // Update FILE_TREE
    if (size == 0)
    {
        // Add a new directory with no files
        FILE_TREE.insert(pair<string, vector<int>>(CURR_DIRECTORY_STRING + name + "/", {}));
    }
    else
    {
        // Update block_list for files
        set_block_list(SUPER_BLOCK->free_block_list, starting_block, starting_block + size, true);
    }

    FILE_TREE[CURR_DIRECTORY_STRING].push_back(inodeID);
    updateSB();
}

void fs_delete(char name[5])
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Print error if file/directory not found
    int inodeID = inodeSearch(name);
    if (inodeID < 0)
    {
        cerr << "Error: File or directory ";
        cerr.write(name, 5) << " does not exist" << endl;
        return;
    }

    // Search for the inodes to delete
    // List of inodes to zero out and delete
    vector<int> inodeList;
    // Find child inodes for directory
    // Directory = 1, File = 0
    if (bitset<8>(SUPER_BLOCK->inode[inodeID].dir_parent).test(7))
    {
        // Directory
        childInodes(SUPER_BLOCK, inodeList, inodeID);
        inodeList.push_back(inodeID);
        // Remove directory and childs from FILE_TREE
        map<string, vector<int>>::iterator it = FILE_TREE.begin();
        string directory = CURR_DIRECTORY_STRING + string(SUPER_BLOCK->inode[inodeID].name, 5).c_str() + "/";
        for (; it != FILE_TREE.end(); it++)
        {
            if (directory.compare(0, directory.size(), it->first) == 0)
            {
                FILE_TREE.erase(it);
            }
        }
    }
    else
    {
        // File
        inodeList.push_back(inodeID);
    }

    // Remove file/directory from FILE_TREE
    vector<int>::iterator it = find(FILE_TREE[CURR_DIRECTORY_STRING].begin(), FILE_TREE[CURR_DIRECTORY_STRING].end(), inodeID);
    if (it != FILE_TREE[CURR_DIRECTORY_STRING].end())
    {
        FILE_TREE[CURR_DIRECTORY_STRING].erase(it);
    }

    while (!inodeList.empty())
    {
        int inode = inodeList.back();
        inodeList.pop_back();

        int start = SUPER_BLOCK->inode[inode].start_block;
        int end = start + bitset<7>(SUPER_BLOCK->inode[inode].used_size).to_ulong();

        // Zero out data blocks
        for (int i = start; i < end; i++)
        {
            char emptyBlock[1024];
            memset(emptyBlock, 0, 1024);
            updateBlock(FILE_DESCRIPTOR, emptyBlock, i * BLOCK_SIZE);
        }

        // Update free_block_list
        set_block_list(SUPER_BLOCK->free_block_list, start, end, false);

        // Zero out Inodes
        strncpy(SUPER_BLOCK->inode[inode].name, "", 5);
        SUPER_BLOCK->inode[inode].used_size = 0;
        SUPER_BLOCK->inode[inode].start_block = 0;
        SUPER_BLOCK->inode[inode].dir_parent = 0;
    }
    updateSB();
}

void fs_read(char name[5], int block_num)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Check if file is under the working directory
    int inodeID = inodeSearch(name);
    if (inodeID < 0 || bitset<8>(SUPER_BLOCK->inode[inodeID].dir_parent).test(7))
    {
        cerr << "Error: File " << name << " does not exist" << endl;
        ;
        return;
    }

    // Check block size
    if (block_num >= 0 && (ulong)block_num < bitset<7>(SUPER_BLOCK->inode[inodeID].used_size).to_ulong())
    {
        // Attempt to read the block of the file
        __off_t offset = (SUPER_BLOCK->inode[inodeID].start_block + block_num) * BLOCK_SIZE;
        if (pread(FILE_DESCRIPTOR, BUFFER, BLOCK_SIZE, offset) < 0)
        {
            cerr << "Error: Cannot read from block" << endl;
            return;
        }
    }
    else
    {
        // Block_num < 0 or more than the file size
        cerr << name << " does not have block " << block_num << endl;
    }
}

void fs_write(char name[5], int block_num)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Check if file is under the working directory
    int inodeID = inodeSearch(name);
    if (inodeID < 0 || bitset<8>(SUPER_BLOCK->inode[inodeID].dir_parent).test(7))
    {
        cerr << "Error: File " << name << " does not exist" << endl;
        return;
    }

    // Check block size
    if (block_num >= 0 && (ulong)block_num < bitset<7>(SUPER_BLOCK->inode[inodeID].used_size).to_ulong())
    {
        // Attempt to read the block of the file
        int offset = (SUPER_BLOCK->inode[inodeID].start_block + block_num) * BLOCK_SIZE;
        updateBlock(FILE_DESCRIPTOR, BUFFER, offset);
        // Done
        return;
    }
    else
    {
        // Block_num < 0 or more than the file size
        cerr << name << " does not have block " << block_num << endl;
    }
}

void fs_buff(char buff[1024])
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Fill BUFFER with user's buffer, will override
    memset(BUFFER, 0, 1024);
    strncpy(BUFFER, buff, BLOCK_SIZE);
}

void fs_ls(void)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    map<string, vector<int>>::iterator it = FILE_TREE.find(CURR_DIRECTORY_STRING);
    if (it != FILE_TREE.end())
    {
        printf("%-5s %3d\n", ".", (int)FILE_TREE[CURR_DIRECTORY_STRING].size() + 2);
        printf("%-5s %3d\n", "..", (int)FILE_TREE[back_directory(CURR_DIRECTORY_STRING)].size() + 2);

        vector<int>::iterator int_it = FILE_TREE[CURR_DIRECTORY_STRING].begin();
        for (; int_it != FILE_TREE[CURR_DIRECTORY_STRING].end(); int_it++)
        {
            // Check if directory
            // Directory print size
            if (bitset<8>(SUPER_BLOCK->inode[*int_it].dir_parent).test(7))
            {
                // Directory
                string next_dir = CURR_DIRECTORY_STRING + string(SUPER_BLOCK->inode[*int_it].name, 5).c_str() + "/";
                printf("%-5.5s %3d\n", SUPER_BLOCK->inode[*int_it].name, (int)FILE_TREE[next_dir].size() + 2);
            }
            else
            {
                // File
                printf("%-5.5s %3d KB\n", SUPER_BLOCK->inode[*int_it].name, (int)bitset<8>(SUPER_BLOCK->inode[*int_it].used_size).set(7, 0).to_ulong());
            }
        }
    }
}

void fs_cd(char name[5])
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Check if name is . or ..SUPER_BLOCK->inode[inodeID].used_size;
    string dir_name(name, 5);
    if (strcmp(dir_name.c_str(), ".") == 0)
    {
        // Do nothing
        return;
    }
    else if (strcmp(dir_name.c_str(), "..") == 0)
    {
        // Go back on directory, if root do nothing
        CURR_DIRECTORY_STRING = back_directory(CURR_DIRECTORY_STRING);
        if (strcmp(CURR_DIRECTORY_STRING.c_str(), "root/") == 0)
        {
            CURR_DIRECTORY.reset();
            CURR_DIRECTORY |= bitset<8>("0111111");
        }
        else
        {
            // Look for the inode of this back directory
            string directory = back_directory(CURR_DIRECTORY_STRING);
            vector<int>::iterator it = FILE_TREE[directory].begin();
            string directoryName = tokenize(CURR_DIRECTORY_STRING, "/").back();
            for (;it != FILE_TREE[directory].end(); it++)
            {
                if (strcmp(directoryName.c_str(), SUPER_BLOCK->inode[*it].name) == 0)
                {
                    // Change CURR_DIRECTORY
                    CURR_DIRECTORY.reset();
                    CURR_DIRECTORY |= bitset<8>(*it);
                }
            }
        }
    }
    else
    {
        // If not, going forward to another directory
        string dir = CURR_DIRECTORY_STRING + string(name, 5).c_str() + '/';
        map<string, vector<int>>::iterator it = FILE_TREE.find(dir);

        // Directories are unique
        if (it != FILE_TREE.end())
        {
            // Directory found, Change directory
            int inodeID = inodeSearch(name);
            if (inodeID < 0)
            {
                cerr << "Error: File or directory ";
                cerr.write(name, 5) << " does not exist" << endl;
                return;
            }
            else
            {
                // Change CURR_DIRECTORY
                CURR_DIRECTORY.reset();
                CURR_DIRECTORY |= bitset<8>(inodeID);
                CURR_DIRECTORY_STRING = dir.c_str();
            }
        }
        else
        {
            // Don't change directory
            cerr << "Error: Directory " << name << " does not exist" << endl;
            return;
        }
    }
}

void fs_resize(char name[5], int new_size)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }

    // Recount the empty blocks
    // Get inodes within the current directory
    vector<int>::iterator it = FILE_TREE[CURR_DIRECTORY_STRING].begin();
    int inodeID;
    bool found;
    for (; it != FILE_TREE[CURR_DIRECTORY_STRING].end(); it++)
    {
        // File name matches given name
        // Inode's type is a file, 0
        if (strncmp(SUPER_BLOCK->inode[*it].name, name, 5) == 0 && !bitset<8>(SUPER_BLOCK->inode[*it].dir_parent).test(7))
        {
            inodeID = *it;
            found = true;
            break;
        }
    }

    // No file of given name found
    if (!found)
    {
        cerr << "Error: File " << name << " does not exist" << endl;
        return;
    }

    // Calculate new tail of Inode
    int size = bitset<7>(SUPER_BLOCK->inode[inodeID].used_size).to_ulong();
    int start = SUPER_BLOCK->inode[inodeID].start_block;
    int end = start + size;
    int newEnd = start + new_size;

    // Extend or Shrink
    if (new_size < size)
    {
        // Shrink
        // Zero out data block by writing
        for (int i = newEnd; i < end; i++)
        {
            char emptyBuff[1024];
            memset(emptyBuff, 0, 1024);
            updateBlock(FILE_DESCRIPTOR, emptyBuff, newEnd * BLOCK_SIZE);
        }

        // Modify free_block_list
        set_block_list(SUPER_BLOCK->free_block_list, newEnd, end, false);

        // Assign new values to Inode
        SUPER_BLOCK->inode[inodeID].used_size = (uint8_t)bitset<8>(bitset<8>(new_size) | bitset<8>("10000000")).to_ulong();
    }
    else if (new_size == size)
    {
        // Do nothing
        return;
    }
    else
    {
        // fake delete from the block list to see if we can resize at somewhere else
        set_block_list(SUPER_BLOCK->free_block_list, start, end, false);
        int newStart = empty_block_finder(contigous_count(SUPER_BLOCK->free_block_list), new_size);

        // Nowhere to put it
        if (newStart < 0)
        {
            // Put it back
            set_block_list(SUPER_BLOCK->free_block_list, start, end, true);
            cerr << "Error: File ";
            cerr.write(name, 5) << " cannot expand to size " << new_size << endl;
            return;
        }

        // Move the file
        newEnd = newStart + new_size;
        moveDB(FILE_DESCRIPTOR, start, end, newStart, newStart + size);
        set_block_list(SUPER_BLOCK->free_block_list, start, end, false);
        set_block_list(SUPER_BLOCK->free_block_list, newStart, newEnd, true);

        // Assign new values to Inode
        SUPER_BLOCK->inode[inodeID].start_block = newStart;
        SUPER_BLOCK->inode[inodeID].used_size = (uint8_t)bitset<8>(bitset<8>(new_size) | bitset<8>("10000000")).to_ulong();
    }
    updateSB();
}
void fs_defrag(void)
{
    if (!MOUNTED)
    {
        cerr << "Error: No file system is mounted" << endl;
        return;
    }
    map<int, int> contigous_blocks = contigous_count(SUPER_BLOCK->free_block_list);
    map<int, int>::iterator it = contigous_blocks.begin();
    while (contigous_blocks.size() > 1)
    {
        // All inodes
        for (int i = 0; i < 126; i++)
        {
            if (it->second <= SUPER_BLOCK->inode[i].start_block)
            {
                // Move this block
                int start = SUPER_BLOCK->inode[i].start_block;
                int size = (int)bitset<7>(SUPER_BLOCK->inode[i].used_size).to_ulong();
                int end = start + size;
                int newStart = start - it->first;
                int newEnd = end - it->first;
                SUPER_BLOCK->inode[i].start_block = newStart;
                moveDB(FILE_DESCRIPTOR, start, end, newStart, newEnd);
                set_block_list(SUPER_BLOCK->free_block_list, start, end, false);
                set_block_list(SUPER_BLOCK->free_block_list, newStart, newEnd, true);
            }
        }
        // Reset
        updateSB();
        contigous_blocks = contigous_count(SUPER_BLOCK->free_block_list);
        it = contigous_blocks.begin();
    }
    updateSB();
}

void fs_free()
{
    delete SUPER_BLOCK;
}