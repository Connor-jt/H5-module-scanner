

#include <iostream>
#include "ModuleStructs.h"
#include "./oodle/oodle.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cassert>
using namespace std;
using namespace ModuleStructs;

std::vector<std::string> module_files;
// sourced from: https://stackoverflow.com/a/25640066/22277207
void FindModules(const std::wstring& directory) {
    std::wstring tmp = directory + L"\\*";
    std::string dir(directory.begin(), directory.end());
    WIN32_FIND_DATAW file;
    HANDLE search_handle = FindFirstFileW(tmp.c_str(), &file);
    if (search_handle != INVALID_HANDLE_VALUE) {
        std::vector<std::wstring> directories;

        do {
            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if ((!lstrcmpW(file.cFileName, L".")) || (!lstrcmpW(file.cFileName, L"..")))
                    continue;
                directories.push_back(directory + L"\\" + std::wstring(file.cFileName));
                continue;
            }
            wstring wide_filename = std::wstring(file.cFileName);
            string filename(wide_filename.begin(), wide_filename.end());
            if (filename.substr(filename.find_last_of(".") + 1) == "module")
                module_files.push_back(dir + "\\" + filename);

        } while (FindNextFileW(search_handle, &file));

        FindClose(search_handle);

        for (std::vector<std::wstring>::iterator iter = directories.begin(), end = directories.end(); iter != end; ++iter)
            FindModules(*iter);
    }
}


class h5_module {
public:
    module_header* header = nullptr;
    module_file* files = nullptr;
    char* string_table = nullptr;
    uint32_t* resource_indexes = nullptr;
    block_header* blocks = nullptr;
    ifstream module_reader = {};
    //uint64_t module_metadata_size; // used to offset when referencing datablock offsets
    //Oodle* unpacker;
    h5_module(string path) {
        // open module file
        module_reader.open(path, ios::binary | ios::ate);
        if (!module_reader.is_open()) {
            throw exception("failed to open filestream");
        }
        streamsize file_size = module_reader.tellg();
        if (file_size < module_header_size) {
            throw exception("filestream too small");
        }
        module_reader.seekg(0, ios::beg);

        // read the header partition
        header = new module_header();
        module_reader.read((char*)header, module_header_size);

        // module file verification // 'mohd', but backwards & h5 forge module version
        if (header->Head != 0x64686F6D) throw exception("target file does not appear to be module file");
        if (header->Version != 27 && header->Version != 23) throw exception("module version does not match target version");

        // read file partition
        files = (module_file*)(new char[module_file_size * header->FileCount]);
        module_reader.read((char*)files, module_file_size * header->FileCount);

        // read string table
        string_table = new char[header->StringsSize];
        module_reader.read(string_table, header->StringsSize);

        // read resource indicies partition
        resource_indexes = new uint32_t[header->ResourceCount];
        module_reader.read((char*)resource_indexes, 4 * header->ResourceCount);

        // read blocks partition
        //blocks = new block_header[header->BlockCount];
        //module_reader.read((char*)blocks, block_header_size * header->BlockCount);

        //uint32_t file_offset = module_header_size + (module_file_size * header->FileCount) + (4 * header->ResourceCount) + (block_header_size * header->BlockCount);
        //module_metadata_size = (file_offset / 0x1000 + 1) * 0x1000; // for some reason 343 aligns the metadata by 0x1000

        //module_reader.release();
    }
    ~h5_module() {
        delete header;
        delete[] files;
        delete[] string_table;
        //delete[] resource_indexes;
        //delete[] blocks;
    }
};



int main()
{
    int test1 = sizeof(module_header);
    int test2 = sizeof(module_file);
    int test3 = sizeof(block_header);


    assert(sizeof(module_header) == module_header_size, "");
    assert(sizeof(module_file) == module_file_size, "");
    assert(sizeof(block_header) == block_header_size, "");
   
    std::cout << "Hello World!\n";

    std::string directory;
    std::getline(std::cin, directory);
    std::cout << "finding modules...\n";
    // fallback so the FindModules function doesn't break when i hit enter twice or something
    struct stat s;
    int err = stat(directory.c_str(), &s);
    if (-1 == err) {
        std::cout << "bad directory.\n";
        return 0;
    }

    wstring wide_directory(directory.begin(), directory.end());
    FindModules(wide_directory);
    std::cout << module_files.size() << " modules found\n\n";
    if (module_files.size() == 0) {
        std::cout << "no modules found.\n";
        return 0;
    }

    for (int p = 0; p < module_files.size(); p++) {
        string mod_path = module_files[p];
        h5_module* mod_file = new h5_module(mod_path);
        for (int i = 0; i < mod_file->header->FileCount; i++) {
            auto tag = mod_file->files[i];

            if (tag.AssetId == 0xDE2F30A8D75E9D0C || tag.AssetChecksum == 0xDE2F30A8D75E9D0C) {
                cout << "found checksum match: TagIndex '" << i << "' Module '" << mod_path << "' tag '" << std::string(mod_file->string_table+tag.NameOffset) << "'\n\n";
            }
        }
        delete mod_file; // heck you john c++, no matter what i do to the code it just comes up with a new error EVERY SINGLE TIME about why i cant possibly close this dumb thing 
    }
}


/// D:\___ Halo Workfolder\midship\server\ng43r_midship_remix