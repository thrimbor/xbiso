/*

xbiso v0.6.1, xdvdfs iso extraction utility developed for linux
Copyright (C) 2003  Tonto Rostenfaunt	<xbiso@linuxmail.org>

Portions dealing with FTP access are
Copyright (C) 2003  Stefan Alfredsson	<xbiso@alfredsson.org>

Other contributors.
Capelle Benoit	<capelle@free.fr>
Rasmus Rohde	<rohde@duff.dk> 

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
//#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include "xbiso.h"
#include "config.h"
#ifdef USE_FTP
#include <ftplib.h>
#endif


const char *version = "0.6.1";
unsigned char dtbuf[4];

struct	dirent {
  unsigned short	ltable;				//left entry
  long	rtable;				//right entry
  long	sector;				//sector of file
  long	size;				//size of file
  unsigned char	attribs;			//file attributes
  unsigned char	fnamelen;			//filename lenght
  char	*fname;				//filename
  long	pad;				//padding (unused)
};
typedef struct dirent TABLE;

void err(char *);
void extract(TABLE *);
void procdir(TABLE *);

#ifdef USE_FTP
int ftp=0;
#endif
int help=0,ret,verb=0;

FILE *xiso;
int xisocompat=0;
void *buffer;

#ifdef _WIN32
typedef off_t	OFFT;
#endif
#ifdef _BSD
typedef off_t OFFT;
#else
typedef off64_t OFFT;
#endif


int main(int argc, char *argv[]) {
  char *dbuf,*fname;
  long dtable, dtablesize;
  int diri=0,ret;
  OFFT cpos;
#ifdef USE_FTP
  char *host, *user, *pass, *initdir;
#endif


  //redefine fseek/ftell per platform
#ifdef _WIN32
  int (*xbfseek)(); xbfseek = fseek;
  OFFT (*xbftell)(); xbftell = ftell;
#endif
#ifdef _BSD
  OFFT (*xbftell)();
  int (*xbfseek)();
  xbfseek = fseeko;
  xbftell = ftello;
#else
  int (*xbfseek)();
  OFFT (*xbftell)();
  xbfseek = fseeko64;
  xbftell = ftello64;
#endif

  if(argv[1] == NULL) { help=1; err(argv[0]); }

  fname = malloc(strlen(argv[1])+1);
  memset(fname,0,strlen(argv[1])+1);
  dbuf = malloc(1);
  memset(dbuf, 0, 1);
  strcpy(fname,argv[1]);
  
#ifdef USE_FTP
  while((ret = getopt(argc,argv,"h:u:p:i:fxvd:")) != -1)
#else
    while((ret = getopt(argc,argv,"xvd:")) != -1)
#endif
      {
	switch(ret) {
	case 'v':
	  verb=1;
	  break;
	case 'x':
	  xisocompat=1;
	  break;
#ifdef USE_FTP
	case 'f':
	  ftp=1;
	  break;				
	case 'h':
	  if(!ftp) { help=1;err(argv[0]); }
	  host = optarg == NULL ? NULL : strdup(optarg);
	  break;
	case 'u':
	  if(!ftp) { help=1;err(argv[0]); }
	  user = optarg == NULL ? NULL : strdup(optarg);
	  break;
	case 'p':
	  if(!ftp) { help=1;err(argv[0]); }
	  pass = optarg == NULL ? NULL : strdup(optarg);
	  /* try to hide pass in ps output */
	  memset(optarg, 0, (size_t)(strlen(optarg))+1);
	  break;
	case 'i':
	  if(!ftp) { help=1;err(argv[0]); }
	  initdir = optarg == NULL ? NULL : strdup(optarg);
	  break;
#endif
	case 'd':
	  dbuf = realloc(dbuf, (size_t)(strlen(optarg))+1);
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
    dbuf = realloc(dbuf, (size_t)(strlen((fname))));
    memset(dbuf, 0, (size_t)(strlen((fname))));
    snprintf(dbuf,(size_t)(strlen(fname))-3,"%s",fname);
  }
  
  buffer = malloc(0x400);
  memset(buffer,0,0x400);
  
  
#ifdef USE_FTP
  if(ftp) {
    if (!ftpOpen(host)) {
      fprintf(stderr,"Unable to connect to node. %s",ftplib_lastresp);
      exit(-1);
    }
    if (!ftpLogin(user,pass)) {
      fprintf(stderr,"Login failure\n");
      exit(-1);
    }
    if (ftpChdir(initdir)!=1) {
      fprintf(stderr,"Login failure\n");
      exit(-1);
    }
    printf("FTP connected to ftp://%s:*****@%s/%s\n", user, host, initdir);
  }
#endif
    

#ifdef _WIN32
  xiso = fopen(fname, "rb");
#endif
#ifdef _BSD
  xiso = fopen(fname, "r");
#else
  xiso = fopen64(fname, "r");
#endif

  if(xiso==NULL) err("Error opening file.\n");

    
#ifdef USE_FTP
  if(ftp) {
    if ( ftpMkdir(dbuf) != 1)  err("Failed to create root directory on FTP\n");
    /* it may already exist, try chdir into it */
    if ( ftpChdir(dbuf) != 1)  err("Failed to change into root directory on FTP\n");
  } else {
#endif
	
#ifdef _WIN32
    CreateDirectory(dbuf,NULL);
    SetCurrentDirectory(dbuf);
#else
    if((mkdir(dbuf,0755))!=0) perror("Failed to create root directory");
    chdir(dbuf);
#endif
#ifdef USE_FTP
  }
#endif

  xbfseek(xiso, (OFFT)0x10000, SEEK_SET);
  fread(buffer, 0x14, 1, xiso);					//header
  if( (strncmp(XMEDHEAD,buffer,0x14)) != 0) printf("Error file doesnt appear to be a xbox iso image\n");


  fread(dtbuf, 1, 4, xiso);				//Sector that root directory table resides in
  dtable = btoll(dtbuf);	
  fread(dtbuf, 1, 4, xiso);				//Size of root directory table in bytes
  dtablesize = btoll(dtbuf);
  xbfseek(xiso, (OFFT)xbftell(xiso)+8, SEEK_SET);			//discard FILETIME structure representing image creation time
  xbfseek(xiso, (OFFT)xbftell(xiso)+0x7c8, SEEK_SET);		//discard unused
  fread(buffer, 0x14, 1, xiso);					//header tail
  if( (strncmp(buffer,XMEDHEAD,0x14)) != 0) err("Error possible corruption?\n"); //check end
  
  
  handlefile((OFFT)dtable*2048, dtable);
  
  printf("End of archive\n");
  fclose(xiso);
  free(buffer);
  free(dbuf);
  free(fname);
#ifdef USE_FTP
  if(ftp) {
    free(host); free(user); free(pass); free(initdir);
  }
#endif
  exit(0);
}

handlefile(OFFT offset, int dtable) {
  TABLE dirent;
    
  //redefine fseek/ftell per platform
#ifdef _WIN32
  int (*xbfseek)(); xbfseek = fseek;
  OFFT (*xbftell)(); xbftell = ftell;
#endif
#ifdef _BSD
  int (*xbfseek)();
  OFFT (*xbftell)();
  xbfseek = fseeko;
  xbftell = ftello;
#else
  int (*xbfseek)();
  OFFT (*xbftell)();
  xbfseek = fseeko64;
  xbftell = ftello64;
#endif

  fseek(xiso, offset, SEEK_SET);
    
  //time to fill the structure
  fread(dtbuf, 1, 2, xiso);			//ltable offset from current
  dirent.ltable = btols(dtbuf);
  fread(dtbuf, 1, 2, xiso);			//rtable offset from current
  dirent.rtable = btols(dtbuf);
  fread(dtbuf, 1, 4, xiso);			//sector of file
  dirent.sector = btoll(dtbuf);
  fread(dtbuf, 1, 4, xiso);				//filesize
  dirent.size = btoll(dtbuf);
  fread(&dirent.attribs, 1, 1, xiso);			//file attributes
  fread(&dirent.fnamelen, 1, 1, xiso);			//filename length
  dirent.fname = malloc(dirent.fnamelen+1);
  if(dirent.fname == 0) {
    printf("Unable to allocate %d bytes\n",dirent.fnamelen+1);
    return -1;
  }
    
  memset(dirent.fname,0,dirent.fnamelen+1);
  fread(dirent.fname, dirent.fnamelen, 1, xiso);	//filename

  if (strstr(dirent.fname,"..") || strchr(dirent.fname, '/') || strchr(dirent.fname, '\\'))                                                                                                                                          
    {                                                                                                                                                                                                                                
      printf("Filename contains invalid characters");                                                                                                                                                                                
      exit(1);                                                                                                                                                                                                                       
    }     
    
  if(verb) {
    printf("ltable offset: %i\nrtable offset: %i\nsector: %li\nfilesize: %li\nattributes: 0x%x\nfilename length: %i\nfilename: %s\n\n", dirent.ltable, dirent.rtable, dirent.sector, dirent.size, dirent.attribs, dirent.fnamelen, dirent.fname);
  }
    
  //xiso compat
  if(xisocompat) {
    if(dirent.rtable != 0) {
      int cpos = xbftell(xiso);
      fread(buffer,32,1,xiso);
      //check if its the last record in block
      if((memcmp(buffer,"\xff\xff\xff\xff\xff\xff",6))==0) {
	//calc our own rtable
	double b1 = (cpos-(dtable*2048));
	int block = ceil(b1/4/512);
	dirent.rtable = block*512;
      }
      memset(buffer,0,32);
      xbfseek(xiso,(OFFT)cpos,SEEK_SET);
    }
      
  }
  //end xiso compat
    
  //check for dirs with no files
  if(dirent.fnamelen != 0) {
    if ((dirent.attribs & FILE_DIR) != 0) {
      procdir(&dirent);

      handlefile((OFFT)dirent.sector*2048, dirent.sector);
	  
#ifdef USE_FTP
      if(ftp) {
	ftpChdir("..");
      } else {
#endif
#ifdef _WIN32
	SetCurrentDirectory("..");
#else
	chdir("..");
#endif
#ifdef USE_FTP
      }
#endif
    } else {
      // Not a directory - just extract it
      extract(&dirent);
    }
    free(dirent.fname);

    if(dirent.rtable != 0)
      handlefile((OFFT)dtable*2048+(dirent.rtable*4), dtable);
    if (dirent.ltable != 0)
      handlefile((OFFT)dtable*2048+(dirent.ltable*4), dtable);
  }
    
}
	
	
void err(char *error) {

  if(help == 1) {
    printf("\n \
		xbiso ver %s %s, Copyright (C) 2003 Tonto Rostenfaunt \n \
		xbiso comes with ABSOLUTELY NO WARRANTY; \n \
		This is free software, and you are welcome to redistribute \n \
		it under certain conditions; See the GNU General Public \n \
		License for more details. \n \
		\n\n",version,platform);

    printf("%s filename [options]\n",error);
    printf("		-v	verbose\n");
    printf("		-x	enable xiso <=1.10 compat\n");
    printf("		-d	(dirname)   directory to extract to\n");
#ifdef USE_FTP
    printf("\n		-f	enable ftp extraction\n");
    printf("		   -h hostname    Address of xbox/ftp server\n");
    printf("		   -u user        User id (normally 'xbox')\n");
    printf("		   -p password    Password\n");
    printf("		   -i path        Start path on server (i.e. '/F/games')\n\n");
#endif
    exit(0);
  }
  printf("%s", error);
  exit(1);
}


void procdir(TABLE *dirent) {
  printf("Creating directory %s\n", dirent->fname);
#ifdef USE_FTP
  if(ftp) {
    if (ftpMkdir(dirent->fname) != 1) err ("Failed to create directory.\n");
    if (ftpChdir(dirent->fname) != 1) err ("Failed to change directory.\n");
  } else {
#endif

#ifdef _WIN32
    CreateDirectory(dirent->fname, NULL);
    SetCurrentDirectory(dirent->fname);
#else
    if((mkdir(dirent->fname, 0755))!=0) err("Failed to create directory.\n");
    chdir(dirent->fname);
#endif

#ifdef USE_FTP
  }
#endif

}

void extract(TABLE *dirent) {
  FILE	*outf;
  void	*fbuf;
  long	rm;

#ifdef USE_FTP
  int c;
  netbuf *nData;
#endif


  //redefine fseek/ftell per platform
#ifdef _WIN32
  typedef off_t	OFFT;
  int (*xbfseek)(); xbfseek = fseek;
  OFFT (*xbftell)(); xbftell = ftell;
#endif
#ifdef _BSD
  int (*xbfseek)();
  OFFT (*xbftell)();
  xbfseek = fseeko;
  xbftell = ftello;
#else
  int (*xbfseek)();
  off64_t (*xbftell)();
  xbfseek = fseeko64;
  xbftell = ftello64;

#endif


  if((long)dirent->size >= (long)BUFFSIZE) {
    fbuf = malloc(BUFFSIZE+1);
    memset(fbuf,0,BUFFSIZE+1);
  } else {
    //quick write
    fbuf = malloc(dirent->size);
    memset(fbuf,0,dirent->size);
    printf("Extracting file %s\n", dirent->fname);
#ifdef USE_FTP
    if(ftp) {
      FtpAccess(dirent->fname, FTPLIB_FILE_WRITE, FTPLIB_IMAGE, DefaultNetbuf, &nData);
    } else {
#endif
#ifdef _WIN32
      outf = fopen(dirent->fname, "wb");
#else
      outf = fopen(dirent->fname,"w");
#endif

#ifdef USE_FTP
    }
#endif
    xbfseek(xiso, (OFFT) dirent->sector*2048, SEEK_SET);
    fread(fbuf, dirent->size, 1, xiso);
#ifdef USE_FTP
    if(ftp) {
      if ( (c = FtpWrite(fbuf, dirent->size, nData)) != dirent->size)
	printf("FtpWrite only wrote %d of %li bytes.\n", c, dirent->size);
      FtpClose(nData);
    } else {
#endif
      fwrite(fbuf, dirent->size, 1, outf);
      fclose(outf);
#ifdef USE_FTP
    }
#endif
    free(fbuf);
    return;

  }
  rm = dirent->size;

  printf("Extracting file %s\n", dirent->fname);
#ifdef USE_FTP
  if(ftp) {
    FtpAccess(dirent->fname, FTPLIB_FILE_WRITE, FTPLIB_IMAGE, DefaultNetbuf, &nData);
  } else {
#endif
#ifdef _WIN32
    outf = fopen(dirent->fname, "wb");
#else
    outf = fopen(dirent->fname,"w");
#endif
#ifdef USE_FTP
  }
#endif

  xbfseek(xiso, (OFFT)dirent->sector*2048, SEEK_SET);

  while(rm!=0) {
    if(rm > BUFFSIZE) {
      rm -= BUFFSIZE;
      fread(fbuf, BUFFSIZE, 1, xiso);

#ifdef USE_FTP
      if(ftp) {
	if ( (c = FtpWrite(fbuf, BUFFSIZE, nData)) != BUFFSIZE)
	  printf("FtpWrite only wrote %d of %li bytes.\n", c, dirent->size);
      } else {
#endif
	fwrite(fbuf, BUFFSIZE, 1, outf);
#ifdef USE_FTP
      }
#endif
    } else {
      memset(fbuf,0,rm+1);
      fread(fbuf, rm, 1, xiso);
#ifdef USE_FTP
      if(ftp) {
	if ( (c = FtpWrite(fbuf, rm, nData)) != rm)
	  printf("FtpWrite only wrote %d of %li bytes.\n", c, rm);
      } else {
#endif
	fwrite(fbuf, rm, 1, outf);
#ifdef USE_FTP
      }
#endif
      rm -= rm;
    }
  }
#ifdef USE_FTP
  if(ftp) {
    FtpClose(nData);
  } else {
#endif
    fclose(outf);
#ifdef USE_FTP
  }
#endif
  free(fbuf);
  return;


}
