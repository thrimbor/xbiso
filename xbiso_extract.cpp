/*
 * xbiso
 * Copyright (C) 2022 Stefan Schmidt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include <iostream>
#include <string>
#include "xbiso.hpp"
#include "xdvdfs.hpp"

#if defined _WIN32
    #include "direct.h"
    #define mkdir(a,b) _mkdir(a)
    #define chdir(a) _chdir(a)
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif


void handleDirectoryEntry (std::ifstream& file, xdvdfs::DirectoryEntry& dirent);

void extractISO (const std::string &filename, const std::string &dirname)
{
    std::cout << "extracting " << filename << " to " << dirname << std::endl;

    std::ifstream isofile;
    isofile.open(filename.c_str(), std::ifstream::binary | std::ifstream::in);
    if (!isofile.is_open()) {
        throw std::runtime_error(std::string("ERROR: Could not open file '") + filename + "'");
    }

    isofile.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

    xdvdfs::VolumeDescriptor vd;
    vd.readFromFile(isofile);

    try {
        vd.validate();
    } catch (xdvdfs::Exception &e) {
        // Try again with the sector offset for Redump files
        vd.readFromFile(isofile, 0x30600);
        vd.validate();
        std::cout << "Redump-style ISO detected" << std::endl;
    }
    vd.validate();

    if (!dryRun) {
        mkdir(dirname.c_str(), 0755);
        chdir(dirname.c_str());
    }

    xdvdfs::DirectoryEntry de = vd.getRootDirEntry(isofile);
    handleDirectoryEntry(isofile, de);

    if (!dryRun)
        chdir("..");
}

void handleDirectoryEntry (std::ifstream& file, xdvdfs::DirectoryEntry& dirent)
{
    // MS tools don't set the content offset to zero for empty directories, instead, they
    // create a padding sector filled with 0xFF. We need to check for this, or we'll get corruption
    if (dirent.isEmptySector())
        return;

    if (dirent.isDirectory())
    {
        std::cout << "creating directory " << dirent.getFilename() << std::endl;

        if (!dryRun) {
            mkdir(dirent.getFilename().c_str(), 0755);
            chdir(dirent.getFilename().c_str());
        }

        xdvdfs::DirectoryEntry de = dirent.getFirstEntry(file);
        handleDirectoryEntry(file, de);

        if (!dryRun)
            chdir("..");
    }
    else
    {
        std::cout << "extracting " << dirent.getFilename() << std::endl;

        if (!dryRun)
        {
            std::ofstream efile;
            efile.open(dirent.getFilename().c_str(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

            if (!efile.is_open()) {
                std::cerr << "failed to open file '" << dirent.getFilename() << "'" << std::endl;
            }

            dirent.extractFile(file, efile);
        }
    }

    if (dirent.hasLeftChild())
    {
        xdvdfs::DirectoryEntry de = dirent.getLeftChild(file);
        handleDirectoryEntry(file, de);
    }

    if (dirent.hasRightChild())
    {
        xdvdfs::DirectoryEntry de = dirent.getRightChild(file);
        handleDirectoryEntry(file, de);
    }
}
