# FS-Sim (File System simulator in C++)

## Usage

`./create_fs` creates a clean disk object.

To start the file system simulator, enter `./fs_sim <disk_name>` in terminal.
Within `./fs_sim` you can input a list of commands:
- `M <disk name>`: Mount the file system residing on the disk
- `C <file name> <file size>`: Create file or directory (size = 0)
- `D <file name>`: Delete file or directory
- `R <file name> <block number>`: Read file at Nth-Block
- `W <file name> <block number>`: Write file at Nth-Block
- `B <new buffer characters>`: Update buffer (up to 1024 characters)
- `L`: List files and directories in current directory
- `E <file name> <new size>`: Change files size 
- `O`: Defragment the disk
- `Y <directory name>`: Change the current working directory

## System Calls:

System call `open` is used in `fs_mount` in `FileSystem.cpp` to open the disk so that we can perform `fs` operations on the disk.

-----
## Function Design:
### `fs_defrag()`
The program checks for empty blocks using `contigous_blocks()`. If `contigous_blocks()` returns a map with size 1 means that the free_block_list has one block that is contigous. We move each inode forward and recalculate until there is only one contigous block left.

### `fs_mount()`
The provided disk name will be used to open a disk in the current working directory of the program. If that works, we then read the first block (1024 bytes) which contains the superblock of our disk.

A consistency check of the disk will then be performed. On failure, we will not modify the global variables such as the file descriptor, super_block, disk_name, file_tree and mounted status. On success, we will update global variables. Then, the current working directory is set to `root/` regardless of whether the disk was mounted.

### `fs_create()`
The mounted status will be tested if there is any disk mounted.
We first check the provided name matches any file or directory in the current directory. If it does, the program will return an error saying that the name has already been taken. If not, we try to find a contigous block to allocate our file. If we are creating a directory, we do not need to find contigous blocks. Next, we find a inode to hold information about the file or directory. If everything succeeds, we will update the global superblock (inode and free_block_list) and the file tree.

### `fs_delete()`
The mounted status will be tested if there is any disk mounted.
Similar to `fs_create()`, the program will check if the provided name exists in the current working directory.
If it is a file, the program will just delete the file by zeroing out values in its occupied data blocks and update the superblock and file tree.
If it is a directory, the program will recursively search for child inodes of this directory and append them onto a vector so that we can erase the inodes and its occupied data blocks.

### `fs_read()`
The mounted status will be tested if there is any disk mounted.
Similar to `fs_create()`, the program will check if the provided name exists in the current working directory.
Additionally, the program will make sure that the name provided is a file instead of a directory.
If all of the criterias match, the program will read the block-th of the file onto the global buffer so that it can be used elsewhere.

### `fs_write()`
The mounted status will be tested if there is any disk mounted.
Similar to `fs_create()`, the program will check if the provided name exists in the current working directory.
Additionally, the program will make sure that the name provided is a file instead of a directory.
If all of the criterias match, the program will write the global buffer onto the block-th of the file.

### `fs_buff()`
The global buffer will copy the contents of the provided buffer.

### `fs_ls()`
The mounted status will be tested if there is any disk mounted.
Prints the name of the inodes that are in the current working directory.
Ii the inode is a directory, the program will print the number of contents that are in the inode. If the inode is a file, the program will print the size of the file.

### `fs_cd()`
The mounted status will be tested if there is any disk mounted.
The program will check if the provided name is already a directory in the global file tree. If true, the program change the global string that is the current working directory to the new directory.

### `fs_resize()`
The mounted status will be tested if there is any disk mounted.
Similar to `fs_create()`, the program will check if the provided name exists in the current working directory.
Additionally, the program will make sure that the name provided is a file instead of a directory.
If all criterias match, the program will either shrink or extend depending on the given size. If the given size is smaller than the size that has been set for the specific inode, the program will shrink by reducing the use size of the inode and zeroing out previously occupied data blocks. If the given size is larger than the size that has been set for the specific inode, the program will attempt to extend by simulating a delete and reallocating the file giving the inode a new position that is more suitable.

-----
## Additional functions:
### Filesystem.cpp
- `inodeSearch`: Returns an inodeID with a given name in the current working directory
- `updateSB`: Write superblock into disk

### Helper.cpp
- `tokenize`: String tokenizer
- `back_directory`: Returns a string of the parent directory of a given director
- `contigous_count`: Returns a map of empty block sizes and its starting block
- `bitset_block_list`: Returns free_block_list in binary form (128 bits)
- `set_block_list`: Sets block[start, end] to a given boolean value in the free_block_list
- `empty_block_finder`: Returns a start block that is contigously empty for at least size large
- `updateBlock`: Write buffer into disk at a certain offset
- `moveDB`: Move data blocks from [start, end] to [newStart, newEnd]

### FSHelper.cpp
- `check1`: Consistency Check 1
- `check2`: Consistency Check 2
- `check3`: Consistency Check 3
- `check4`: Consistency Check 4
- `check5`: Consistency Check 5
- `check6`: Consistency Check 6
- `ccheck`: Consistency Check (1-6)
- `deserializeSB`: Superblock deserializer
- `buildFS`: Returns a map of directories with inodeIDs that exists in their respective directory
- `childInodes`: Adds parent inode's child inodes into a vector


-----
## Testing:

As for testing the correctness of the code, `Filesystem.cpp` , `Helper.cpp` , `FSHelper.cpp` and its respective header files were tested with the tests cases provided by the Lab TAs. The sample test cases in `sample_tests` and faulty disks in `consistency-check` was used to test for correctness. 
The correctness was determined by the output of the program. The standard output and standard error of the program was compared with the stdout and stderr provided in the sample test cases. Furthermore, the disk(s) that was modified by the program was compared to the `disk_result`(s) in the test case using a Hex Editor (e.g. Hexdump and GHex).

-----
## Sources:

C++ Map:

http://www.cplusplus.com/reference/map/map/

*By The C++ Resources Network, 2019*



C++ Vector:

http://www.cplusplus.com/reference/vector/vector/

*By The C++ Resources Network, 2019*



C++ String:

http://www.cplusplus.com/reference/string/string/

*By The C++ Resources Network, 2019*



C++ Bitset:

http://www.cplusplus.com/reference/bitset/

*By The C++ Resources Network, 2019*


C++ ifstream:

http://www.cplusplus.com/reference/fstream/ifstream/

*By The C++ Resources Network, 2019*


Portions of this project are modifications based on work created and shared by the Lab Instructors:

String Tokenizer, General Information and Bitwise Operations

[*Assignment 1 Startercode*](https://eclass.srv.ualberta.ca/mod/resource/view.php?id=3803904), [*Lab 9*](https://eclass.srv.ualberta.ca/mod/resource/view.php?id=3898629) and [*Bitwise Operations*](https://eclass.srv.ualberta.ca/mod/resource/view.php?id=3898736)
