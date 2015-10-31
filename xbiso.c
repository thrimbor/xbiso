/*

xbiso v0.5.6, xdvdfs iso extraction utility developed for linux

Copyright (C) 2002  Tonto Rostenfaunt	xbiso@linuxmail.org

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

This program would not have been developed if it were not for the
xdvdfs documentation provided by the xbox-linux project.
http://xbox-linux.sourceforge.net

*/

#ifdef _WIN32
	#include <win.h>
	#include "getopt.h"

	const char *platform = "win32";
#else
	#define _LARGEFILE64_SOURCE
	#include <unistd.h>
	const char *platform = "linux";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "xbiso.h"

const char *version = "0.5.6";


struct	dirent {
	int	ltable;				//left entry
	int	rtable;				//right entry
	long	sector;				//sector of file
	long	size;				//size of file
	int	attribs;			//file attributes
	int	fnamelen;			//filename lenght
	char	*fname;				//filename
	long	pad;				//padding (unused)
};
typedef struct dirent TABLE;


void err(char *);
void extract(int);
void procdir(int);

int help = 0;
TABLE dirent[MAX_DEPTH];
FILE *xiso;

#ifdef _WIN32
	typedef off_t	OFFT;
#else
	typedef off64_t OFFT;
#endif


int main(int argc, char *argv[]) {

	void *buffer;
	char *dbuf,*fname;
	long dtable[MAX_DEPTH],dtablesize[MAX_DEPTH];
	int ltable=0,rtable=0,diri=0,ret,verb=0;
	OFFT cpos;

	//redefine fseek/ftell per platform
	#ifdef _WIN32
		int (*xbfseek)(); xbfseek = fseek;
		OFFT (*xbftell)(); xbftell = ftell;
	#else
		int (*xbfseek)();
		OFFT (*xbftell)();
		xbfseek = fseeko64;
		xbftell = ftello64;
	#endif

	if(argv[1] == NULL) { help=1; err(argv[0]); }

	fname = malloc(strlen(argv[1]));
	memset(fname,0,strlen(argv[1]));
	dbuf = malloc(1);
	memset(dbuf, 0, 1);

	fname = argv[1];

	while((ret = getopt(argc,argv,"vd:")) != -1) {
		switch(ret) {
			case 'v':
				verb=1;
			break;
			case 'd':
				realloc(dbuf, (size_t)(strlen(optarg))+1);
				memset(dbuf, 0, (size_t)(strlen(optarg))+1);
				snprintf(dbuf,(size_t)(strlen(optarg))+1,"%s",optarg);
			break;
			case '?':
				help = 1;
				err(argv[0]);
			break;
		}
	}

	//set the dirname to the filename - ext if blank
	if(strcmp(dbuf,"")==0) {
		realloc(dbuf, (size_t)(strlen((fname)-3)));
		memset(dbuf, 0, (size_t)(strlen((fname)-3)));
		snprintf(dbuf,(size_t)(strlen(fname))-3,"%s",fname);
	}


	buffer = malloc(0x400);
	memset(buffer,0,0x400);

	#ifdef _WIN32
		xiso = fopen(fname, "rb");
	#else
		xiso = fopen64(fname, "r+");
	#endif

	if( xiso == NULL ) err("Error opening file.\n");

	#ifdef _WIN32
		CreateDirectory(dbuf,NULL);
		SetCurrentDirectory(dbuf);
	#else
		if((mkdir(dbuf,0755))!=0) err("Failed to create root directory\n");
		chdir(dbuf);
	#endif

	free(dbuf);


	xbfseek(xiso, (OFFT)0x10000, SEEK_SET);
	fread(buffer, 0x14, 1, xiso);					//header
	if( (strncmp(buffer,XMEDHEAD,0x14)) != 0) err("Error file doesnt appear to be a xbox iso image\n");


	fread(&dtable[diri], 4, 1, xiso);				//Sector that root directory table resides in
	fread(&dtablesize[diri], 4, 1, xiso);				//Size of root directory table in bytes
	xbfseek(xiso, (OFFT)xbftell(xiso)+8, SEEK_SET);			//discard FILETIME structure representing image creation time
	xbfseek(xiso, (OFFT)xbftell(xiso)+0x7c8, SEEK_SET);			//discard unused
	fread(buffer, 0x14, 1, xiso);					//header tail
	if( (strncmp(buffer,XMEDHEAD,0x14)) != 0) err("Error possible corruption?\n"); //check end
	xbfseek(xiso, (OFFT)dtable[diri]*2048, SEEK_SET);


	//main loop
	while(1) {
		//time to fill the structure
		fread(&dirent[diri].ltable, 2, 1, xiso);			//ltable offset from current
		fread(&dirent[diri].rtable, 2, 1, xiso);			//rtable offset from current
		fread(&dirent[diri].sector, 4, 1, xiso);			//sector of file
		fread(&dirent[diri].size, 4, 1, xiso);				//filesize
		fread(&dirent[diri].attribs, 1, 1, xiso);			//file attributes
		fread(&dirent[diri].fnamelen, 1, 1, xiso);			//filename lenght
		dirent[diri].fname = malloc(dirent[diri].fnamelen+1);
		memset(dirent[diri].fname,0,dirent[diri].fnamelen+1);
		fread(dirent[diri].fname, dirent[diri].fnamelen, 1, xiso);	//filename


		if(verb) {
		printf("ltable offset: %li\nrtable offset: %li\nsector: %i\nfilesize: %li\nattributes: 0x%x\nfilename lenght: %i\nfilename: %s\n\n", dirent[diri].ltable, dirent[diri].rtable, dirent[diri].sector, dirent[diri].size, dirent[diri].attribs, dirent[diri].fnamelen, dirent[diri].fname);
		}

		if((dirent[diri].attribs & FILE_NOR) !=0) {
			cpos = xbftell(xiso);
			extract(diri);
			xbfseek(xiso, (OFFT)cpos, SEEK_SET);					//reset position
			xbfseek(xiso, (OFFT)dtable[diri]*2048, SEEK_SET);			//seek back to table start
			xbfseek(xiso, (OFFT)xbftell(xiso)+(dirent[diri].rtable*4), SEEK_SET);	//seek to next entry

		} else if ((dirent[diri].attribs & FILE_DIR) != 0) {
			procdir(diri);
			xbfseek(xiso, (OFFT)dirent[diri].sector*2048, SEEK_SET);		//seeking to next tree
			dtable[diri+1] = dirent[diri].sector;				//set sector of new table
			diri++;
			dirent[diri].rtable = 1;

		} else {
				// 0x01 FILE_RO, 0x02 FILE_HID, 0x04 FILE_SYS, 0x20 FILE_ARC
				// Not going to bother setting other attributes for now
                        cpos = xbftell(xiso);
                        extract(diri);
                        xbfseek(xiso, (OFFT)cpos, SEEK_SET);                                    //reset position
                        xbfseek(xiso, (OFFT)dtable[diri]*2048, SEEK_SET);                       //seek back to table start
                        xbfseek(xiso, (OFFT)xbftell(xiso)+(dirent[diri].rtable*4), SEEK_SET);     //seek to next entry
		}

			while(dirent[diri].rtable == 0 && dirent[diri].ltable == 0) {
				if(diri == 0) {
					printf("End of archive\n");
					free(buffer);
					exit(0);
				}
				#ifdef _WIN32
					SetCurrentDirectory("..");
				#else
					chdir("..");
				#endif

				free(dirent[diri].fname);
				diri--;
				xbfseek(xiso, (OFFT)dtable[diri]*2048, SEEK_SET);			//seek to last table
				xbfseek(xiso, (OFFT)xbftell(xiso)+(dirent[diri].rtable*4), SEEK_SET);     //seek to next entry
			}
	}

	return 0;
}

void err(char *error) {

	if(help == 1) {
		printf("\n \
		xbiso ver %s %s, Copyright (C) 2002 Tonto Rostenfaunt \n \
		xbiso comes with ABSOLUTELY NO WARRANTY; \n \
		This is free software, and you are welcome to redistribute \n \
		it under certain conditions; See the GNU General Public \n \
		License for more details. \n \
		\n\n",version,platform);

		printf("%s filename [options]\n",error);
		printf("		-v		verbose\n");
		printf("		-d (dirname)	directory to extract to\n");
		exit(0);
	}
	printf("%s", error);
	exit(1);
}


void procdir(int diri) {
	#ifdef _WIN32
		CreateDirectory(dirent[diri].fname, NULL);
		SetCurrentDirectory(dirent[diri].fname);
	#else
		if((mkdir(dirent[diri].fname, 0755))!=0) err("Failed to create directory.\n");
		chdir(dirent[diri].fname);
	#endif
}

void extract(int diri) {
	FILE	*outf;
	void	*fbuf;
	long	rm;

	//redefine fseek/ftell per platform
	#ifdef _WIN32
		typedef off_t	OFFT;
		int (*xbfseek)(); xbfseek = fseek;
		OFFT (*xbftell)(); xbftell = ftell;
	#else
		int (*xbfseek)();
		off64_t (*xbftell)();
		xbfseek = fseeko64;
		xbftell = ftello64;

	#endif


	if((long)dirent[diri].size >= (long)BUFFSIZE) {
		fbuf = malloc(BUFFSIZE+1);
		memset(fbuf,0,BUFFSIZE+1);
	} else {
		//quick write
		fbuf = malloc(dirent[diri].size);
		memset(fbuf,0,dirent[diri].size);
		#ifdef _WIN32
			outf = fopen(dirent[diri].fname, "wb");
		#else
			outf = fopen(dirent[diri].fname,"w+");
		#endif
		xbfseek(xiso, (OFFT) dirent[diri].sector*2048, SEEK_SET);
		fread(fbuf, dirent[diri].size, 1, xiso);
		fwrite(fbuf, dirent[diri].size, 1, outf);
		fclose(outf);
		free(fbuf);
		return;

	}
	rm = dirent[diri].size;

	#ifdef _WIN32
		outf = fopen(dirent[diri].fname, "wb");
	#else
		outf = fopen(dirent[diri].fname,"w+");
	#endif

	xbfseek(xiso, (OFFT)dirent[diri].sector*2048, SEEK_SET);

	while(rm!=0) {
		rm -= BUFFSIZE;
		if(rm > BUFFSIZE) {
			fread(fbuf, BUFFSIZE, 1, xiso);
			fwrite(fbuf, BUFFSIZE, 1, outf);
		} else {
			rm += BUFFSIZE;

			#ifdef _WIN32		//for some reason realloc is causing problems w/ w2k/xp
				free(fbuf);
				fbuf = malloc(rm+1);
				memset(fbuf,0,rm+1);
			#else
				realloc(fbuf,rm+1);
				memset(fbuf,0,rm+1);
			#endif
			fread(fbuf, rm, 1, xiso);
			fwrite(fbuf, rm, 1, outf);
			rm -= rm;
		}
	}
	fclose(outf);
	free(fbuf);
	return;


}
