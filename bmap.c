/* bmap.c userlevel blockmap utility for Linux.
 *
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10227 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 * Written 1998,1999 by Daniel Ridge in support of:
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 */

/* config.h can contain preprocessor directives controlling system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "mft.h"

#include "bmap.h"

/*
 * flags for major operating modes
 */
enum operating_modes {BMAP_MAP,BMAP_CARVE,BMAP_SLACK,BMAP_PUTSLACK,BMAP_WIPESLACK,BMAP_CHECKSLACK,BMAP_SLACKBYTES,BMAP_WIPE,BMAP_FRAGMENT,BMAP_CHECKFRAG};
enum doc_modes {BMAP_VERSION,BMAP_HELP,BMAP_MAN,BMAP_SGML};

static int flag_mode=BMAP_MAP;	/* what operation are we going to perform */
static int flag_raw=1;		/* use raw device access for carve,wipe,slack */
static int flag_label=0;	/* print info about carved slack space */
static int flag_filename=0;	/* flag to print filename after first sector */

static struct mft_option options[]={
	{"doc","autogenerate document ...",
		MOT_VENUM|MOF_SILENT,
		MO_VENUM_CAST{
			{"version","display version and exit",
				0,MO_INT_CAST(BMAP_VERSION)
			},
			{"help","display options and exit",
				0,MO_INT_CAST(BMAP_HELP)
			},
			{"man","generate man page and exit",
				MOF_HIDDEN,MO_INT_CAST(BMAP_MAN)
			},
			{"sgml","generate SGML invocation info",
			 	MOF_HIDDEN,MO_INT_CAST(BMAP_SGML)
			},
			{NULL,NULL,0,MO_CAST(NULL)}
		}
	},
	{"mode","operation to perform on files",
		MOT_VENUM|MOF_SILENT,
		MO_VENUM_CAST{
			{"map","list sector numbers",
				0,MO_INT_CAST(BMAP_MAP)},
			{"carve","extract a copy from the raw device",
				0,MO_INT_CAST(BMAP_CARVE)},
			{"slack","display data in slack space",
				0,MO_INT_CAST(BMAP_SLACK)},
			{"putslack","place data into slack",
				0,MO_INT_CAST(BMAP_PUTSLACK)},
			{"wipeslack","wipe slack",
				0,MO_INT_CAST(BMAP_WIPESLACK)},
			{"checkslack","test for slack (returns 0 if file has slack)",
				0,MO_INT_CAST(BMAP_CHECKSLACK)},
			{"slackbytes","print number of slack bytes available",0,MO_INT_CAST(BMAP_SLACKBYTES)},
			{"wipe","wipe the file from the raw device",0,MO_INT_CAST(BMAP_WIPE)},
			{"frag","display fragmentation information for the file",0,MO_INT_CAST(BMAP_FRAGMENT)},
			{"checkfrag","test for fragmentation (returns 0 if file is fragmented)",0,MO_INT_CAST(BMAP_CHECKFRAG)},
			{NULL,NULL,0,MO_CAST(NULL)}}
		},
	{"outfile","write output to ...",MOT_FILENAME,{NULL}},
	{"label","useless bogus option",MOT_FLAG,{NULL}},
	{"name","useless bogus option",MOT_FLAG,{NULL}},
	{"verbose","be verbose",MOT_FLAG,{NULL}},
	{"log-thresh","logging threshold ...",
		MOT_ENUM,
		MO_ENUM_CAST{
			"none",
			"fatal",
			"error",
			"info",
			"branch",
			"progress",
			"entryexit",
			NULL
		}
	},
	{"target","operate on ...",MOT_FILENAME|MOF_SILENT,{NULL}},
	{0,0,0,{0}}
};

static struct mft_info bmap_info={
	"bmap",
	"use block-list knowledge to perform special operations on files",
	AUTHOR,
	VERSION " (" BUILD_DATE ")",
	options
};

int
main(int argc,char **argv)
{
int target_fd=0;	/* fd of target file */
int outfile_fd=0;
int raw_fd=0;		/* device fd for raw operations */
int block=0;
int prev_block=-1;
int block_size=0;	
long filesize=0;	/* size of file in bytes per stat */
int retval=0;
int read_count=0;
int block_count=0;
int x;
struct stat target_statval;		/* stat against target file */
unsigned char *block_buffer=NULL;	
char *filename=NULL;
int raw_file_mode=0;
char *outfile_filename=NULL;
int log_thresh;
off_t offset;
int slack_bytes;

int optval;
int option_index;
union option_arg option_arg;

	/* initialize the logger */
	mft_log_init();
	mft_log_entry();

	if(sizeof(off_t)!=8)
	{
		mft_logf(MLOG_FATAL,"off_t too small!");
		mft_log_exit();
		exit(12);
	}

	/* parse some command-line arguments */
	option_index=0;
	optval=0;
	while(optval != -EINVAL && (optval=mft_getopt(argc,argv,options,MO_REMOVE,&option_index,(void *)&option_arg)) != -ENOENT)
	{
		switch(optval)
		{
		case 0: /* doc */
			switch(option_arg.a_enum)
			{
			case BMAP_VERSION:
			  mft_display_version(stdout,&bmap_info);
			  break;
			case BMAP_HELP:
			  mft_display_version(stdout,&bmap_info);
			  mft_display_help(stdout,&bmap_info,NULL);
			  break;
			case BMAP_MAN:
			  mft_display_man(stdout,BUILD_DATE,1,&bmap_info,NULL);			  
			  break;
			case BMAP_SGML:
			  mft_display_sgml(stdout,&bmap_info,NULL);
			  break;
			}
			mft_log_exit();
			exit(0);
			break;
		case 1: /* mode */
			flag_mode=option_arg.a_enum;
			break;
		case 2: /* outfile */
			/* '-' implies stdout which is default */
			if(strcmp("-",option_arg.a_string))
				outfile_filename=strdup(option_arg.a_string);
			break;
		case 3: /* label */
			flag_label=option_arg.a_flag;
			break;
		case 4: /* name */
			flag_filename=option_arg.a_flag;
			break;
		case 5: /* verbose */
			mft_log_set(MLOG_INFO);
			break;
		case 6: /* log-thresh */
			log_thresh=option_arg.a_enum;
			if(log_thresh==0)
				log_thresh=MLOG_NONE;
			else if(log_thresh==1)
				log_thresh=MLOG_FATAL;
			else if(log_thresh==2)
				log_thresh=MLOG_ERROR;
			else if(log_thresh==3)
				log_thresh=MLOG_INFO;
			else if(log_thresh==4)
				log_thresh=MLOG_BRANCH;
			else if(log_thresh==5)
				log_thresh=MLOG_PROGRESS;
			else if(log_thresh==6)
				log_thresh=MLOG_ENTRYEXIT;
			mft_log_push(log_thresh);
			break;
		case 7: /* target */
			filename=option_arg.a_filename;
			break;
		case -EINVAL: /* an option we don't understand */
			/* is this a non-option arg */
			if(option_arg.a_invalid && *option_arg.a_invalid=='-')
			{
				mft_logf(MLOG_FATAL,"invalid option: %s",option_arg.a_invalid);
				mft_log(MLOG_FATAL,"try '--help' for help.");
				mft_log_exit();
				exit(12);
			}
			break;
		default:
			mft_logf(MLOG_ERROR,"how did we get here?");
			break;
		}
	}

	/*
	 * bmap requires a filename.
	 */
	if(optval==-ENOENT || optval!=-EINVAL)
	{
		mft_log(MLOG_FATAL,"no filename. try '--help' for help.");
		mft_log_exit();
		exit(2);
	} else {
		filename=option_arg.a_invalid;
	}

	mft_logf(MLOG_PROGRESS,"target filename: %s",filename);

	if((target_fd=lstat(filename,&target_statval))==-1)
	{
		mft_logf(MLOG_FATAL,"Unable to stat file: %s\n",filename);
		mft_log_exit();
		exit(3);
	}

	/* blockmap only makes sense on regular files */
	if(!S_ISREG(target_statval.st_mode) || S_ISLNK(target_statval.st_mode))
	{
		mft_logf(MLOG_INFO,"%s is not a regular file.",filename);
		mft_log_exit();
		exit(4);
	}

	if(target_statval.st_nlink>1)
	{
		mft_logf(MLOG_INFO,"%s has multiple links.",filename);
	}

	if(outfile_filename)
	{
		if(((outfile_fd=open(outfile_filename,O_APPEND | O_CREAT | O_WRONLY,0755))==-1))
		{
			mft_logf(MLOG_FATAL,"Unable to open file: %s",outfile_filename);
			mft_log_exit();
			exit(11);
		}
	} else {
		outfile_fd=1;
	}

	if((target_fd=open(filename,O_RDONLY,0))==-1)
	{
		mft_logf(MLOG_FATAL,"Unable to open file: %s",filename);
		mft_log_exit();
		exit(5);
	}

	if((block_size=bmap_get_block_size(target_fd))==-1)
	{
		mft_logf(MLOG_FATAL,"Unable to determine blocksize");
		mft_log_exit();
		exit(7);
	}
	mft_logf(MLOG_PROGRESS,"target file block size: %d",block_size);

	if(flag_mode!=BMAP_MAP && flag_mode!=BMAP_FRAGMENT && flag_mode!=BMAP_CHECKFRAG)
	{
		if(flag_raw)
		{
			if(flag_mode==BMAP_WIPE || flag_mode==BMAP_PUTSLACK || flag_mode==BMAP_WIPESLACK)
				raw_file_mode=O_WRONLY;
			else
				raw_file_mode=O_RDONLY;

			if((raw_fd=bmap_raw_open(filename,raw_file_mode))==-1)
			{
				mft_logf(MLOG_FATAL,"unable to raw open %s",filename);
				mft_log_exit();
				exit(6);
			}
		}
	}

	if((block_count=bmap_get_block_count(target_fd,&target_statval))==-1)
	{
		mft_logf(MLOG_FATAL,"Unable to determine count");
		mft_log_exit();
		exit(8);
	}
	
	/* most operations require a <= blocksize buffer */
	if(flag_mode!=BMAP_MAP)
	{
		if(!(block_buffer=(unsigned char *)malloc(block_size)))
		{
			mft_logf(MLOG_FATAL,"Unable to allocate buffer");
			mft_log_exit();
			exit(9);
		}
	}

	filesize=target_statval.st_size;

	/* is the file size larger than the reported blocks could contain? */
	if(block_count*block_size < filesize)
	{
		mft_logf(MLOG_INFO,"%s has holes in excess of %ld bytes...",filename,filesize-(block_count*block_size));
	}

	/*
	 * main action loop. carve,wipe,map all want to walk over
	 * all the blocks.
	 */

	for(x=0;x<block_count;x++)
	{
		if((block=bmap_map_block(target_fd,x))==-1)
		{
			mft_logf(MLOG_FATAL,"error mapping block %d (%s)",x,strerror(errno));
			exit(124);
		} else if(!block) {
			mft_logf(MLOG_INFO,"nul block while mapping block %d.",x);
		} else if(flag_mode==BMAP_CARVE) {
			offset=((long long)block)*block_size;
			if(lseek(raw_fd,offset,SEEK_SET)!=offset)
			{
				mft_logf(MLOG_ERROR,"seek failure");
			} else {
				read_count=read(raw_fd,block_buffer,block_size);
				if(read_count<block_size)
				{
					mft_logf(MLOG_ERROR,"read error");
				}
				read_count=write(outfile_fd,block_buffer,block_size);
				if(read_count<block_size)
				{
					mft_logf(MLOG_ERROR,"write error");
				}
			}
		} else if(flag_mode==BMAP_WIPE) {
			offset=((long long)block)*block_size;
			bogowipe(raw_fd,offset,block_size,block_buffer,block_size);
		} else if(flag_mode==BMAP_FRAGMENT) {
			if((prev_block != -1) && (block != prev_block+1))
			{
				dprintf(outfile_fd,"%s fragmented between %d and %d\n",
				  filename,
				  (prev_block+1)*(block_size/512),
				  block*(block_size/512)-1);
			}
			prev_block=block;
		} else if(flag_mode==BMAP_CHECKFRAG) {
			if((prev_block != -1) && (block != prev_block+1))
			{
				retval=2;
				break;
			}
			prev_block=block;
		} else if(flag_mode==BMAP_MAP) {
		int y;

			for(y=0;y<block_size/512;y++)
			{
				if(flag_filename && (x==0) && (y==0))
				{
					dprintf(outfile_fd,"%d %s\n",
					  block*(block_size/512)+y,
					  filename);
				}
				else
					dprintf(outfile_fd,"%d\n",
					  block*(block_size/512)+y);
			}
		}
	}

	/*
	 * the 'slack' family of operations are only concerned about the
	 * terminal block.
	 */
	offset=((long long)block)*block_size+(filesize%block_size);
	if(!(filesize%block_size))
		slack_bytes=0;
	else
		slack_bytes=block_size-filesize%block_size;
	if(flag_mode==BMAP_SLACK && block!=0)
	{
		mft_logf(MLOG_INFO,"getting from block %d",block);
		mft_logf(MLOG_INFO,"file size was: %ld",filesize);
		mft_logf(MLOG_INFO,"slack size: %d",slack_bytes);
		mft_logf(MLOG_INFO,"block size: %d",block_size);

		if(lseek(raw_fd,offset,SEEK_SET)!=offset) {
			mft_logf(MLOG_ERROR,"seek error");
		} else {
			if(flag_label)
				dprintf(outfile_fd,"# File: %s  Location: %Ld  size: %d\n",
				filename,(long long)(block*block_size+(filesize%block_size)),slack_bytes);
	
			read(raw_fd,block_buffer,slack_bytes);
			write(outfile_fd,block_buffer,slack_bytes);
		}
	} else if(flag_mode==BMAP_PUTSLACK && block!=0) {
		mft_logf(MLOG_INFO,"stuffing block %d",block);
		mft_logf(MLOG_INFO,"file size was: %ld",filesize);
		mft_logf(MLOG_INFO,"slack size: %d",slack_bytes);
		mft_logf(MLOG_INFO,"block size: %d",block_size);

		/* grab new slack material from stdin and pump into file */
		if(lseek(raw_fd,offset,SEEK_SET)!=offset) {
			mft_logf(MLOG_ERROR,"seek error");
		} else {
			slack_bytes=read(0,block_buffer,slack_bytes);
			write(raw_fd,block_buffer,slack_bytes);
		}
	} else if(flag_mode==BMAP_WIPESLACK && block!=0) {
		mft_logf(MLOG_INFO,"stuffing block %d",block);
		mft_logf(MLOG_INFO,"file size was: %ld",filesize);
		mft_logf(MLOG_INFO,"slack size: %d",slack_bytes);
		mft_logf(MLOG_INFO,"block size: %d",block_size);

		bogowipe(raw_fd,offset,slack_bytes,block_buffer,block_size);
	} else if(flag_mode==BMAP_CHECKSLACK) {
		retval=1;
		if(block)
		{
			if(lseek(raw_fd,offset,SEEK_SET)!=offset) {
				mft_logf(MLOG_ERROR,"seek error");
			} else {
				slack_bytes=read(raw_fd,block_buffer,slack_bytes);
				for(x=0;x<slack_bytes;x++)
				{
					if(block_buffer[x])
						retval=0;
				}
			}
		}
	} else if(flag_mode==BMAP_SLACKBYTES) {
		if(block)
			dprintf(outfile_fd,"%d\n",(int)(block_size-filesize%block_size));
		else
			dprintf(outfile_fd,"%d\n",0);
	}

	close(target_fd);
	close(raw_fd);
	
	if(outfile_fd!=1)
		close(outfile_fd);

	if(block_buffer) free(block_buffer);
	
	if(flag_mode!=BMAP_CHECKSLACK && flag_mode!=BMAP_CHECKFRAG)
	{
		retval=0;
	} else if(flag_mode==BMAP_CHECKSLACK) {
		if(retval==0) {
			mft_logf(MLOG_INFO,"%s has slack",filename);	
		} else if(retval==1) {
			mft_logf(MLOG_INFO,"%s does not have slack",filename);
		}
	} else if(flag_mode==BMAP_CHECKFRAG) {
		if(retval==2) {
			mft_logf(MLOG_INFO,"%s has fragmentation",filename);	
			retval=0;
		} else {
			mft_logf(MLOG_INFO,"%s does not have fragmentation",filename);
			retval=1;
		}
	}
 
	return retval;
}
