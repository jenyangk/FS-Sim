#include <stdio.h>
#include <stdint.h>

typedef struct {
	char name[5];        // Name of the file or directory
	uint8_t used_size;   // Inode state and the size of the file or directory
	uint8_t start_block; // Index of the start file block
	uint8_t dir_parent;  // Inode mode and the index of the parent inode
} Inode;

typedef struct {
	char free_block_list[16];
	Inode inode[126];
} Super_block;

/**
 * Mounts the file system residing on the virtual disk with the specified name. The mounting process involves
loading the superblock of the file system, but before doing this, you should check if there exists a file (i.e.,
a virtual disk) with the given name in the current working directory. Print the following error to stderr if
this file does not exist:
Error: Cannot find disk <disk name>
The next step is to check the consistency of the file system. We say a file system is inconsistent when an
arbitrary number of bits in its superblock are changed accidentally. Your program should read through the
file system and perform the following consistency checks:
1. Blocks that are marked free in the free-space list cannot be allocated to any file. Similarly, blocks
marked in use in the free-space list must be allocated to exactly one file.
2. The name of every file/directory must be unique in each directory.
3. If the state of an inode is free, all bits in this inode must be zero. Otherwise, the name attribute stored
in the inode must have at least one bit that is not zero.
4. The start block of every inode that is marked as a file must have a value between 1 and 127
inclusive.
5. The size and start block of an inode that is marked as a directory must be zero.
6. For every inode, the index of its parent inode cannot be 126. Moreover, if the index of the parent inode
is between 0 and 125 inclusive, then the parent inode must be in use and marked as a directory.
If the file system is inconsistent, you must print the following error to stderr:
Error: File system in <disk name> is inconsistent (error code: <number>)
where <number> identifies the type of inconsistency (as listed above) and can be from 1 to 6. Note that you
must check the file system inconsistency in the same order as above, and only report the first inconsistency
you detect. Hence, if a file system is inconsistent for several different reasons, you only print the smallest
error code.
Your program should not mount a file system that fails the consistency check. In this case, you should
continue to use the last file system that was successfully mounted for running the next commands. If no file
system was successfully mounted before, you should print the following error to stderr for each command
that comes after the mount command until you read another mount command that can mount a file system
successfully:
Error: No file system is mounted
Note that you should report this error only if the command is valid but no file system is mounted. If the
command is not valid, you should just print the command error (described earlier).
If the disk exists and the residing file system is consistent, you can proceed to loading the superblock and set
the current working directory to the root directory. Do not flush the buffer when mounting a file system.
 * 
*/
void fs_mount(char *new_disk_name);

/**
 * Creates a new file or directory in the current working directory with the given name and the given number
of blocks, and stores the attributes in the first available inode. A size of zero means that the user is creating
a directory. If no inode is available, you must print the following error to stderr:
4
Error: Superblock in disk <disk name> is full, cannot create <file name>
You must make sure the new file or directory have a unique name within the current working directory, and
the name cannot be . or .. which is reserved. You can have multiple files with the same name only if they
are located in different directories. If there already exists a file or directory with the same name under the
current working directory, then print the following error to stderr:
Error: File or directory <file name> already exists
Recall that the file must be allocated a number of contiguous blocks, so your program must find the first
set of contiguous blocks that can be allocated to the file by scanning data blocks from 1 to 127. Print the
following error to stderr if there is not enough contiguous free blocks to allocate to the new file:
Error: Cannot allocate <file size> on <disk name>
You should check the availability of a free inode, then the uniqueness of the file/directory name, and finally
the availability of enough contiguous free blocks to fit the new file’s data blocks.

 */ 
void fs_create(char name[5], int size);

/**
 * Deletes the file or directory with the given name in the current working directory. If the name represents a
directory, your program should recursively delete all files and directories within this directory. For every file
or directory that is deleted, you must zero out the corresponding inode and data block. Do not shift other
inodes or file data blocks after deletion. If the specified file or directory is not found in the current working
directory, print the following error to stderr:
Error: File or directory <file name> does not exist

 */ 
void fs_delete(char name[5]);

/**
 * Opens the file with the given name and reads the block num-th block of the file into the buffer. If no such
file exists or the given name corresponds to a directory under the current working directory, the following
error must be printed to stderr:
Error: File <file name> does not exist
If the block num is not in the range of [0, size-1], where size is the number of blocks allocated to the
file, print the following error to stderr:
Error: <file name> does not have block <block_num>
 */ 
void fs_read(char name[5], int block_num);

/**
 * Opens the file with the given name and writes the content of the buffer to the block num-th block of the
file. If no such file exists or the given name corresponds to a directory under the current working directory,
the following error must be printed to stderr:
Error: File <file name> does not exist
If the block num is not in the range of [0, size-1], where size is the number of blocks allocated to the
file, print the following error to stderr:
Error: <file name> does not have block <block_num>
 */ 
void fs_write(char name[5], int block_num);

/**
 *•Flushes the buffer by setting it to zero and writes the new bytes into the buffer. No errors must be handled in
 this function.
 */  
void fs_buff(char buff[1024]);
void fs_ls(void);

/**
 * Changes the size of the file with the given name to new size. If no such file exists in the current working
directory or the name corresponds to a directory rather than a file, your program should print the following
error message to stderr:
Error: File <file name> does not exist
6
If the new size is greater than the current size of the file, you need to allocated more blocks to this file.
You must keep the start block fixed and add new data blocks to the end such that the file’s data blocks
are still contiguous. If there are enough free blocks after the last block of this file, change the size in the
inode to new size. 
Otherwise, you must try to move the file so that you can increase the file size to the
new size, and copy all data in the previous blocks to the new blocks. The new starting block must be at the
first available one that has enough space for the resized file. If there are not enough contiguous free blocks
on the disk to fit the file with its new size, print the following error to stderr and do not update the file
size:
Error: File <file name> cannot expand to size <new size>

If the new size is smaller than the current size, then delete and zero out blocks from the tail of the block
sequence allocated to this file. The starting block is not moved when decreasing a files size. Finally, change
the size attribute in the inode to the new size.

Note: You can assume that new_size is greater than zero in fs_resize
 */ 
void fs_resize(char name[5], int new_size);
void fs_defrag(void);

/**
 * Changes the current working directory to a directory with the specified name in the current working directory.
This directory can be ., .., or any directory the user created on the disk. If the specified directory does
not exist in the current working directory (i.e., the name does not exist or it points to a file rather than a
directory), print the following error message to stderr:
Error: Directory <directory name> does not exist
You can assume that the given name has no slash at the end.
 */ 
void fs_cd(char name[5]);
void fs_free();