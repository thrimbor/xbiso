/*
 * xbiso
 * Copyright (C) 2015 Stefan Schmidt
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

#include "xdvdfs.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "optionparser.h"
#include <xbisoConfig.h>

#if defined _WIN32
    #include "direct.h"
    #define mkdir(a,b) _mkdir(a)
    #define chdir(a) _chdir(a)
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

void handleDirectoryEntry (std::ifstream& file, xdvdfs::DirectoryEntry& dirent);

struct Arg: public option::Arg {
    static option::ArgStatus NonEmpty (const option::Option& option, bool msg) {
        if (option.arg)
            return option::ARG_OK;

        if (msg)
            std::cerr << "Option '" << option.name << "' requires an argument" << std::endl;

        return option::ARG_ILLEGAL;
    }
};

enum optionIndex {UNKNOWN, HELP, VERBOSE, EXTRACT, DRYRUN, PROGRESS, DIRECTORY};
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, ""},
    {HELP, 0, "h", "help", option::Arg::None, ""},
    {VERBOSE, 0, "v", "verbose", option::Arg::None, ""},
    {EXTRACT, 0, "x", "extract", option::Arg::None, ""},
    {DRYRUN, 0, "n", "dry-run", option::Arg::None, ""},
    {PROGRESS, 0, "p", "progress", option::Arg::None, ""},
    {DIRECTORY, 0, "d", "directory", Arg::NonEmpty, ""},
    {0,0,0,0,0,0}
};

int verbosityLevel = 0;
bool dryRun = false;

void printUsage ()
{
    std::cout << "\n"
              << "xbiso " << XBISO_VERSION << "-" << XBISO_OS << ", Copyright (C) 2015 Stefan Schmidt\n\n"
              << "This program is free software: you can redistribute it and/or modify\n"
              << "it under the terms of the GNU General Public License as published by\n"
              << "the Free Software Foundation, either version 3 of the License, or\n"
              << "(at your option) any later version.\n\n"
              << "This program is distributed in the hope that it will be useful,\n"
              << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
              << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
              << "GNU General Public License for more details.\n\n"
              << "You should have received a copy of the GNU General Public License\n"
              << "along with this program.  If not, see <http://www.gnu.org/licenses/>."
              << std::endl;

    std::cout << "\n"
              << "Usage: xbiso [options] file...\n"
              << "Options:\n"
              << "  -h,--help              Print this help message\n"
              << "  -v,--verbose           Be verbose\n"
              << "  -x,--extract           Extract the passed image files\n"
              << "  -n,--dry-run           Dry-run only, don't actually modify files\n"
              << "  -p,--progress          Show progress while extracting/creating\n"
              << "  -d,--directory <dir>   Extract into directory <dir>.\n"
              << "                         Only valid when passing a single file.\n"
              << std::endl;
}

int main (int argc, char* argv[])
{
    argc -= (argc>0); argv += (argc>0);

    option::Stats stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

    if (parse.error())
        return 1;

    if (options[DIRECTORY] && parse.nonOptionsCount() > 1) {
        std::cerr << "ERROR: Specifying an extraction directory is not allowed when passing more than one image file." << std::endl;
        return 1;
    }

    if (options[HELP] || argc == 0) {
        printUsage();
        return 0;
    }

    if (options[DRYRUN])
        dryRun = true;

    if (options[EXTRACT]) {

        for (int i=0; i<parse.nonOptionsCount(); ++i) {
            std::string filename = parse.nonOption(i);
            std::string dirname = options[DIRECTORY] ? options[DIRECTORY].arg : filename.substr(0, filename.find_last_of("."));

            std::cout << "extracting " << filename << " to " << dirname << std::endl;

            std::ifstream isofile;
            isofile.open(filename.c_str(), std::ifstream::binary | std::ifstream::in);
            if (!isofile.is_open()) {
                std::cerr << "ERROR: Could not open file '" << filename << "'" << std::endl;
                return 1;
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
    } else {
        printUsage();
    }

    return 0;
}

void handleDirectoryEntry (std::ifstream& file, xdvdfs::DirectoryEntry& dirent)
{
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
