#include <cstdio>
#include <cstdint>

#include <fstream>

#include "../logger/logger.h"
#include "file.h"

using namespace std;
using namespace logger;

static string toString(const char* a) 
{
    string s = "";
    while (*a)
        s += *a++;

    return s; 
}

fileManager::~fileManager()
{
    fileHandle *fhandle;

    while (fileList.size())
    {
        fhandle = fileList.front();
        fhandle->~fileHandle();
    }

    log(filemanager, "Destroying file manager");
}

void fileManager::removeFileHandle(fileHandle *handle)
{
    fileList.remove(handle);
}

fileHandle::~fileHandle()
{
    log(filemanager, "Destroying file handle");
    if (_data)
        delete[] _data;

    // Remove from list
    _parent->removeFileHandle(this);
}

fileHandle *fileManager::open(string name)
{
    log(filemanager, "Attempting to open: " + name);
    ifstream file (name, ifstream::binary);
    fileHandle *fhandle;

    if (file)
    {
        file.seekg (0, file.end);
        int size = file.tellg();
        file.seekg (0, file.beg);

        if (size < 1 || size > (16*1024*1024)) {
            log(filemanager, "Filesize or generic error. File NOT read");
            file.close();
            return nullptr;
        }

        uint8_t *data = new uint8_t [size];
        if (!data) {
            log(filemanager, "Could not allocate buffer. File NOT read");
            return nullptr;
        }

        file.read((char*)data, size);
        if (file.gcount() == size) {
            file.close();
            log(filemanager, "Success!");

            fhandle = new fileHandle(this, name,size,data);
            fileList.push_back(fhandle);
            return fhandle;
        }

        log(filemanager, "Could not read file (read: " + to_string(file.gcount()) + ")");
        delete[] data;
        file.close();
    }
    else
        log(filemanager, "Could not open file");

    return nullptr;;
}


void fileManager::write(string name, uint8_t *buf, size_t len)
{
	ofstream file(name, ofstream::binary | ofstream::trunc);

	if (file.is_open())
	{
		file.write((const char*)buf, len);
		if (file.bad())
		{
			file.close();
			log(filemanager, "Could not write file");
			return;
		}
		file.close();
        log(filemanager, "Successfully wrote " + name);
	}
	else
    {
		log(filemanager, "Could not write to: " + name);
    }
}

void fileManager::write(const char *name, uint8_t *buf, size_t len)
{
	ofstream file(name, ofstream::binary | ofstream::trunc);

	if (file.is_open())
	{
		file.write((const char*)buf, len);
		if (file.bad())
		{
			file.close();
			log(filemanager, "Could not write file");
			return;
		}
		file.close();
        log(filemanager, "Successfully wrote " + toString( name ));
	}
	else
    {
		log(filemanager, "Could not write to: " + toString( name ));
    }
}

void fileManager::write(string name, string data)
{
	ofstream file(name, ofstream::binary | ofstream::trunc);

	if (file.is_open())
	{
		file << data;
		if (file.bad())
		{
			file.close();
			log(filemanager, "Could not write file");
			return;
		}
		file.close();
	}
	else
		log(filemanager, "Could not write to: " + name);
}

fileHandle::fileHandle(fileManager *const parent, const std::string name, const int size, uint8_t *const data) :
        name(_name), size(_size), data(_data)
{
    log(filemanager, "Creating file handle");
    _data = data;
    _name = name;
    _size = size;
    _parent = parent;
}
