#ifndef FILE_H
#define FILE_H

#include <cstdio>
#include <cstdint>
#include <string>
#include <list>

class fileManager;

class fileHandle
{
    std::string  _name;
    int          _size;
    uint8_t     *_data;
    fileManager *_parent;

public:
    explicit fileHandle(fileManager *const parent, const std::string name, const int size, uint8_t *const data);
    ~fileHandle();
    const std::string    &name;
    const int            &size;
          uint8_t *const &data;
};

class fileManager {
    friend fileHandle;
public:
    ~fileManager();
    fileHandle *open(std::string name);
	void write(std::string name, uint8_t *, size_t);
    void write(const char *name, uint8_t *, size_t);
	void write(std::string name, std::string data);
protected:
    void removeFileHandle(fileHandle*);
private:
    std::list <fileHandle*> fileList;
};

#endif
