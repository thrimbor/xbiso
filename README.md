# xbiso
xbiso is an ISO extraction and creation utility for XDVDFS images.

## Old version
This new version of xbiso is written in C++ and is NOT directly based on the old C-code written by Tonto Rostenfaust. I'd like to thank him and the other authors for their work, though, because it was their work that got me interested in writing this program. If you want to find out more about the old xbiso-version and it's authors, you can find all information in the history of this repository.


## FAQ ##
### How can I create an image?
Unfortunately, you can't right now. Creating an image is more complicated than extracting one and thus isn't implemented yet.

### How do I extract an image?
Simply call xbiso with the "-x" parameter. To see a list of options supported by xbiso, simply call it without any parameters or with the "-h" parameter.

### What operating systems are supported?
I've been developing and testing this program on both Linux and Windows, both x86_64.
Please not that big-endian architectures aren't supported right now (they were on the old version), I'm currently planning to readd support in a clean way.

### How can I build xbiso myself?
I recommend the following procedure (on Linux):
1. Clone the Git-repository and enter the repository-folder
2. Create a directory in which to build the program and enter it: mkdir build && cd build
3. Run CMake: cmake ../
4. Run make: make
5. Enjoy your very own "xbiso" binary!

### Why the rewrite?
The old code wasn't very pretty and had no good Windows-support. The rewrite in C++ makes it easier to read, understand and extend the code.

### Why doesn't this version have FTP-support?
FTP support would make the whole code more complex while being of little benefit. It isn't very complex or cumbersome to use an external FTP-client. "Do one thing and do it well."
