#include <cstdio>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <fstream>

#include <string>
#include <cstring>
#include <vector>

#include "FileSystem.h"
#include "Helper.h"

using namespace std;

int main(int argc, char *argv[])
{
    // If the user doesn't not provide any input file(s)
    if (argc != 2)
    {
        cerr << "usage: ./fs input_disk" << endl;
        exit(1);
    }
    ifstream disk(argv[1]);
    if (!disk.is_open())
    {
        cerr << "Disk is not opened." << endl;
    }

    // Get arguments
    vector<string> arguments;
    string arg;
    int line_counter = 0;

    while (!disk.eof())
    {
        line_counter++;
        getline(disk, arg);
        arguments = tokenize(arg, " ");

        char command = arg[0];
        switch (command)
        {
        case 'M':
            if (arguments.size() != 2)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_mount((char *)arguments[1].c_str());
            break;
        case 'C':
            if (arguments.size() != 3)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (atoi(arguments[2].c_str()) < 0 || atoi(arguments[2].c_str()) > 127)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_create((char *)arguments[1].c_str(), atoi(arguments[2].c_str()));
            break;
        case 'D':
            if (arguments.size() != 2)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }

            fs_delete((char *)arguments[1].c_str());
            break;
        case 'R':
            if (arguments.size() != 3)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (atoi(arguments[2].c_str()) < 0 || atoi(arguments[2].c_str()) > 127)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_read((char *)arguments[1].c_str(), atoi(arguments[2].c_str()));
            break;
        case 'W':
            if (arguments.size() != 3)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (atoi(arguments[2].c_str()) < 0 || atoi(arguments[2].c_str()) > 127)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_write((char *)arguments[1].c_str(), atoi(arguments[2].c_str()));
            break;
        case 'B':
        {
            if (arguments.size() == 1)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            string::iterator it = arg.begin();
            // Skip the first character
            it++;
            while (*it == ' ')
            {
                it++;
            }
            arg.erase(arg.begin(), it);

            if (arg.size() > 1024)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_buff((char *)arg.c_str());
        }
        break;
        case 'L':
            if (arguments.size() > 1)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_ls();
            break;
        case 'E':
            if (arguments.size() != 3)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if ((atoi(arguments[2].c_str()) > 127))
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }

            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_resize((char *)arguments[1].c_str(), atoi(arguments[2].c_str()));
            break;
        case 'O':
            if (arguments.size() > 1)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_defrag();
            break;
        case 'Y':
            if (arguments.size() != 2)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            if (arguments[1].size() > 5)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
                continue;
            }
            fs_cd((char *)arguments[1].c_str());
            break;
        default:
            if (arg.size() != 0)
            {
                cerr << "Command Error: " << argv[1] << ", " << line_counter << endl;
            }
            break;
        }
    }
    disk.close();
    fs_free();
    return 0;
}
