/* dev_builder.c -- construct the .c support for bmap to understand devs.
 * 
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10277 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 * Written 1999 by Daniel Ridge in support of:
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int
main(int argc,char **argv)
{
DIR *DIR;
struct dirent *dirent;
struct stat statval;
char *dev_dir="/dev";
char *path_buffer;


	if((DIR=opendir(dev_dir))==NULL)
	{
		fprintf(stderr,"Couldn't open %s!\n",dev_dir);
		exit(1);
	}

	printf("/* This file is generated automatically */\n");
	printf("#include <stdlib.h>\n");
	printf("#include \"bmap.h\"\n");
	printf("struct bmap_dev_entry bmap_dev_entries[]={\n");

	while((dirent=readdir(DIR))!=NULL)
	{
		if(strcmp(dirent->d_name,".") && strcmp(dirent->d_name,".."))
		{
			path_buffer=malloc(strlen(dirent->d_name)+strlen(dev_dir)+2);
			sprintf(path_buffer,"%s/%s",dev_dir,dirent->d_name);
			if(lstat(path_buffer,&statval)==-1)
			{
				fprintf(stderr,"Unable to stat file: %s\n",path_buffer);
			} else {
				if(S_ISBLK(statval.st_mode))
				{
					printf("\t{\"%s/%s\",%d,%d},\n",dev_dir,dirent->d_name,(int)(statval.st_rdev>>8),(int)(statval.st_rdev&0xff));
				}
			}
			free(path_buffer);
		}
	}

	printf("\t{NULL,0,0}\n};\n");	

	exit(0);
}
