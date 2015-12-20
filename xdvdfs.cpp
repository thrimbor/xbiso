#include "xdvdfs.hpp"

#include <vector>
#include <cstring>
#include <iostream>

// TODO: support for big endian architectures

void xdvdfs::VolumeDescriptor::readFromFile (std::ifstream& file)
{
    std::vector<char> buffer(2048);

    // read the whole sector
    file.seekg(VOLUME_DESCRIPTOR_SECTOR*SECTOR_SIZE, file.beg);
    file.read(buffer.data(), buffer.size());

    // TODO: couldn't we use the stream operator instead?
    std::copy(buffer.begin(), buffer.begin()+0x14, this->magicNumber);
    std::copy(buffer.begin()+0x14, buffer.begin()+0x18, reinterpret_cast<char*>(&this->rootDirTableSector));
    std::copy(buffer.begin()+0x18, buffer.begin()+0x1C, reinterpret_cast<char*>(&this->rootDirTableSize));
    std::copy(buffer.begin()+0x1C, buffer.begin()+0x24, this->filetime);
    std::copy(buffer.begin()+0x7EC, buffer.end(), this->magicNumber2);

    // endianess conversion
    this->rootDirTableSector = le_to_host(this->rootDirTableSector);
    this->rootDirTableSize = le_to_host(this->rootDirTableSize);
}

void xdvdfs::VolumeDescriptor::validate ()
{
    if (std::memcmp(this->magicNumber, MAGIC_NUMBER, 0x14) != 0)
        throw new xdvdfs::Exception("First magic number incorrect");

    if (this->rootDirTableSector == 0 || this->rootDirTableSize == 0)
        throw new xdvdfs::Exception("Root directory table is empty");

    if (std::memcmp(this->magicNumber2, MAGIC_NUMBER, 0x14) != 0)
        throw new xdvdfs::Exception("Second magic number incorrect");
}

xdvdfs::DirectoryEntry xdvdfs::VolumeDescriptor::getRootDirEntry (std::ifstream& file)
{
    xdvdfs::DirectoryEntry dirent;

    dirent.readFromFile(file, rootDirTableSector);

    return dirent;
}

void xdvdfs::DirectoryEntry::readFromFile (std::ifstream& file, std::streampos sector, std::streampos offset)
{
    std::vector<char> buffer(2048);

    // read the whole sector
    file.seekg(sector*xdvdfs::SECTOR_SIZE + offset, file.beg);
    file.read(buffer.data(), buffer.size());

    std::copy(buffer.begin(), buffer.begin()+0x02, reinterpret_cast<char*>(&this->leftSubTree));
    std::copy(buffer.begin()+0x02, buffer.begin()+0x04, reinterpret_cast<char*>(&this->rightSubTree));
    std::copy(buffer.begin()+0x04, buffer.begin()+0x08, reinterpret_cast<char*>(&this->startSector));
    std::copy(buffer.begin()+0x08, buffer.begin()+0x0C, reinterpret_cast<char*>(&this->fileSize));
    std::copy(buffer.begin()+0x0C, buffer.begin()+0x0D, reinterpret_cast<char*>(&this->attributes));

    // reading the filename requires a bit more work
    uint8_t* filenameLength = reinterpret_cast<uint8_t*>(&buffer[0x0D]);
    this->filename = std::string(&buffer[0x0E], *filenameLength);

    this->sectorNumber = sector;

    // endianess conversion
    this->leftSubTree = le_to_host(this->leftSubTree);
    this->rightSubTree = le_to_host(this->rightSubTree);
    this->startSector = le_to_host(this->startSector);
    this->fileSize = le_to_host(this->fileSize);
}

std::string xdvdfs::DirectoryEntry::getFilename ()
{
    return this->filename;
}

std::streamsize xdvdfs::DirectoryEntry::getFileSize()
{
    return this->fileSize;
}

void xdvdfs::DirectoryEntry::extractFile(std::ifstream& file, std::ofstream& ofile)
{
    if (this->isDirectory())
        throw new xdvdfs::Exception("Tried to access directory as a file");

    std::vector<char> buffer(4096);
    std::size_t filesize = this->fileSize;

    file.seekg(xdvdfs::SECTOR_SIZE * this->startSector);

    while (filesize > 0)
    {
        if (filesize > buffer.size())
        {
            file.read(buffer.data(), buffer.size());
            ofile.write(buffer.data(), buffer.size());
            filesize -= buffer.size();
        }
        else
        {
            file.read(buffer.data(), filesize);
            ofile.write(buffer.data(), filesize);
            filesize = 0;
        }
    }
}

bool xdvdfs::DirectoryEntry::isDirectory ()
{
    return ((this->attributes & xdvdfs::DirectoryEntry::FILE_DIRECTORY) != 0);
}

bool xdvdfs::DirectoryEntry::hasLeftChild ()
{
    return (this->leftSubTree != 0);
}

bool xdvdfs::DirectoryEntry::hasRightChild ()
{
    return (this->rightSubTree != 0);
}

xdvdfs::DirectoryEntry xdvdfs::DirectoryEntry::getLeftChild (std::ifstream& file)
{
    xdvdfs::DirectoryEntry dirent;
    dirent.readFromFile(file, this->sectorNumber, static_cast<std::streampos>(this->leftSubTree*4));

    return dirent;
}

xdvdfs::DirectoryEntry xdvdfs::DirectoryEntry::getRightChild (std::ifstream& file)
{
    xdvdfs::DirectoryEntry dirent;
    dirent.readFromFile(file, this->sectorNumber, static_cast<std::streampos>(this->rightSubTree*4));

    return dirent;
}

xdvdfs::DirectoryEntry xdvdfs::DirectoryEntry::getFirstEntry (std::ifstream& file)
{
    if (!this->isDirectory())
        throw new xdvdfs::Exception("Tried to access file as a directory");

    xdvdfs::DirectoryEntry dirent;
    dirent.readFromFile(file, this->startSector);

    return dirent;
}
