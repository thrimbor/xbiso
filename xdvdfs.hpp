#pragma once

#include <exception>
#include <fstream>

namespace xdvdfs
{
    static const int SECTOR_SIZE = 2048;
    static const int VOLUME_DESCRIPTOR_SECTOR = 32;

    static const char MAGIC_NUMBER[] = "MICROSOFT*XBOX*MEDIA";

    class Exception : public std::exception
    {
        public:
            explicit Exception (const char * str) : sDetails(str) {}

            const char* what() const noexcept override
            {
                return this->sDetails.c_str();
            }
        private:
            std::string sDetails;
    };

    template<typename T>
    T le_to_host(T t)
    {
        uint8_t *bptr = reinterpret_cast<uint8_t *>(&t);
        T r = 0;

        for (std::size_t s=0; s < sizeof(T); ++s)
        {
            r |= bptr[s] << s*8;
        }

        return r;
    }

    class DirectoryEntry;

    /**
     * This class describes the volume descriptor of xdvdfs which is placed at
     * sector 32 of the image. It contains a zero-filled area to fill a whole
     * sector.
    */
    class VolumeDescriptor
    {
        public:
            void readFromFile (std::ifstream& file, const std::ifstream::off_type sectorOffset = 0);
            void validate () const;
            DirectoryEntry getRootDirEntry (std::ifstream& file);

        private:
            char magicNumber[0x14];         ///< 20 byte block containing the magic number
            uint32_t rootDirTableSector;    ///< sector number of the root directory table
            uint32_t rootDirTableSize;      ///< size of the root directory table in bytes
            char filetime[8];               ///< FILETIME-structure containing the creation time
            /* zero filled area to fill the whole sector */
            char magicNumber2[0x14];        ///< 20 byte block containing the same magic number
            std::ifstream::off_type sectorOffset;
    };

    class DirectoryEntry
    {
        public:
            void readFromFile (std::ifstream& file, std::streampos pos, std::streampos offset = 0, std::ifstream::off_type sectorOffset = 0);
            std::string getFilename () const;
            std::streamsize getFileSize() const;
            void extractFile(std::ifstream& file, std::ofstream& ofile);
            bool isDirectory () const;
            bool hasLeftChild () const;
            bool hasRightChild () const;
            DirectoryEntry getLeftChild (std::ifstream& file);
            DirectoryEntry getRightChild (std::ifstream& file);
            DirectoryEntry getFirstEntry (std::ifstream& file);

            static const uint8_t FILE_READONLY  = 0x01;
            static const uint8_t FILE_HIDDEN    = 0x02;
            static const uint8_t FILE_SYSTEM    = 0x04;
            static const uint8_t FILE_DIRECTORY = 0x10;
            static const uint8_t FILE_ARCHIVE   = 0x20;
            static const uint8_t FILE_NORMAL    = 0x80;

        private:
            uint16_t leftSubTree;
            uint16_t rightSubTree;
            uint32_t startSector;
            uint32_t fileSize;
            uint8_t  attributes;
            std::string filename;

            std::ifstream::off_type sectorOffset;
            std::streampos sectorNumber;
    };
}
