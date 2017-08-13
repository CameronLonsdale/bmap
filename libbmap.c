/* libbmap.c helper functions for userlevel blockmap (under Linux).
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
#include <sys/ioctl.h>

#include <linux/fs.h>	/* supplies FIGETBSZ and FIBMAP */

#include "mft.h"

#include "bmap.h"

/*
 * external prototypes
 */
int bmap_get_slack_block(int fd,long *slack_block,long *slack_bytes,long *block_size);
int bmap_get_block_count(int fd,const struct stat *statval);
int bmap_get_block_size(int fd);
int bmap_map_block(int fd,unsigned long block);
int bmap_raw_open(const char *filename,mode_t mode);
void bmap_raw_close(int fd);

/*
 * helper prototypes
 */
long get_terminal_block(int fd);
int bogowipe(int fd,off_t offset,int len,unsigned char *block_buffer,int buffer_len);
char *dev2filename(dev_t dev);

/*
 * flags for major operating modes
 */
enum operating_modes {BMAP_MAP,BMAP_CARVE,BMAP_SLACK,BMAP_PUTSLACK,BMAP_WIPESLACK,BMAP_CHECKSLACK,BMAP_SLACKBYTES,BMAP_WIPE,BMAP_FRAGMENT};

/*
 * NULL value OK for slack_bytes
 */
int
bmap_get_slack_block(int fd,long *slack_block,long *slack_bytes,long *block_size)
{
struct stat statval;
off_t file_size=0;
unsigned long hidden_block_size=0;
unsigned long block_count=0;
unsigned long block;
int retval;

	mft_log_entry();

	if(!block_size)
		block_size=&hidden_block_size;

	if(!slack_block)
	{
		mft_logf(MLOG_ERROR,"NULL value for slack_block");
		mft_log_exit();
		return -1;
	}

	if(fstat(fd,&statval)==-1)
	{
		mft_logf(MLOG_ERROR,"Unable to stat fd");
		mft_log_exit();
		return -1;
	}

	if((*block_size=bmap_get_block_size(fd))==-1)
	{
		mft_logf(MLOG_ERROR,"Unable to determine blocksize");
		mft_log_exit();
		return -1;
	}

	file_size=statval.st_size;
	block_count=bmap_get_block_count(fd,&statval);

	if(block_count==-1)
	{
		mft_logf(MLOG_INFO,"error getting block count");
		mft_log_exit();
		return -1;
	} else if(!block_count) {
		mft_logf(MLOG_INFO,"fd has no blocks");
		mft_log_exit();
		return -1;	
	}

	mft_logf(MLOG_PROGRESS,"mapping block %lu",block_count);
	block=block_count-1;
	retval=ioctl(fd,FIBMAP,&block);
	if(retval)
	{
		mft_logf(MLOG_ERROR,"error mapping block %d. ioctl failed with %s",block_count,strerror(errno));
		mft_log_exit();
		return -1;
	} else if(!block) {
		mft_logf(MLOG_ERROR,"error mapping block %d. block returned 0",block_count);
		mft_log_exit();
		return -1;
	}

	/* fill out the user parameters */
	*slack_block=block;
	if(slack_bytes)
	{
		if(!(file_size%*block_size))
		{
			*slack_bytes=0;
		} else {
			*slack_bytes=*block_size-file_size%*block_size;
		}
	}
	
	mft_log_exit();
	return 0;
}

int
bmap_get_block_count(int fd,const struct stat *statval)
{
struct stat private_statval;
int block_size;
int block_count=0;

	mft_log_entry();

	/* stat parameter is optional */
	if(!statval)
	{
		statval=&private_statval;
		if(fstat(fd,(struct stat *)statval)==-1)
		{
			mft_logf(MLOG_ERROR,"unable to stat fd");
			mft_log_exit();
			return -1;
		}
	}

	/* grab the filesystem block size */
	if((block_size=bmap_get_block_size(fd))==-1)
	{
		mft_logf(MLOG_ERROR,"unable to determine filesystem blocksize");
		mft_log_exit();
		return -1;
	}

	if(!block_size)
	{
		mft_logf(MLOG_ERROR,"filesystem reports 0 blocksize");
		mft_log_exit();
		return -1;
	}

	block_count=statval->st_size/block_size;
	if(statval->st_size%block_size)
		block_count++;

	mft_logf(MLOG_PROGRESS,"computed block count: %d",block_count);
	mft_logf(MLOG_PROGRESS,"stat reports %d blocks: %d",statval->st_blocks);
			

	mft_log_exit();
	return block_count;	
}

int
bmap_get_block_size(int fd)
{
int block_size;

	/* grab the filesystem block size */
	if(ioctl(fd,FIGETBSZ,&block_size)==-1)
	{
		mft_logf(MLOG_ERROR,"unable to determine filesystem blocksize");
		mft_log_exit();
		return -1;
	}

	if(!block_size)
	{
		mft_logf(MLOG_ERROR,"filesystem reports 0 blocksize");
		mft_log_exit();
		return -1;
	}

	return block_size;
}

int
bmap_map_block(int fd,unsigned long block)
{
int block_pos;

	mft_log_entry();

	block_pos=block;
	if(ioctl(fd,FIBMAP,&block_pos)==-1)
	{
		mft_logf(MLOG_ERROR,"error mapping block %d. ioctl failed with %s",block,strerror(errno));
		return -1;
	} else if(!block_pos) {
		mft_logf(MLOG_BRANCH,"nul block while mapping block %d.",block);
		return 0;
	} else {
		return block_pos;
	}
}

int
bmap_raw_open(const char *filename,mode_t mode)
{
struct stat target_statval;
struct stat raw_statval;
char *raw_filename;
int raw_fd=0;

	mft_log_entry();

	if(!filename)
	{
		mft_log(MLOG_ERROR,"NULL filename supplied");
		errno=EINVAL;
		mft_log_exit();
		return -1;
	}

	if(lstat(filename,&target_statval)==-1)
	{
		mft_logf(MLOG_ERROR,"Unable to stat file: %s",filename);
		mft_log_exit();
		return -1;
	}

	/* we only work on regular files */
	if(!S_ISREG(target_statval.st_mode) || S_ISLNK(target_statval.st_mode))
	{
		mft_logf(MLOG_INFO,"%s is not a regular file.",filename);	
		errno=ENOSYS;
		mft_log_exit();
		return -1;
	}

	if(!(raw_filename=dev2filename(target_statval.st_dev)))
	{
		mft_logf(MLOG_ERROR,"unable to determine raw device of %s",filename);
		errno=ENOSYS;
		mft_log_exit();
		return -1;
	}

	if(lstat(raw_filename,&raw_statval)==-1)
	{
		mft_logf(MLOG_ERROR,"unable to stat raw device %s",raw_filename);
		mft_log_exit();
		return -1;
	}

	if(raw_statval.st_rdev!=target_statval.st_dev)
	{
		mft_logf(MLOG_ERROR,"device mismatch 0x%x != 0x%x",target_statval.st_rdev,raw_statval.st_dev);
		errno=ENXIO;
		mft_log_exit();
		return -1;
	}

	if((raw_fd=open(raw_filename,mode,0))==-1)
	{
		mft_logf(MLOG_ERROR,"unable to open raw device %s",raw_filename);
		mft_log_exit();
		return -1;
	}

	mft_logf(MLOG_PROGRESS,"raw fd is %d",raw_fd);
	mft_log_exit();	
	return raw_fd;
}

void
bmap_raw_close(int fd)
{
	mft_log_entry();

	close(fd);

	mft_log_exit();
}


/*
 * We have to try to guess the filename from the dev info. I actually
 * wouldn't mind doing mknod() in tmp, but we may be doing this as
 * part of a console review process and not wish to scribble on the disk.
 */

char *
dev2filename(dev_t dev)
{
int major,minor;
struct bmap_dev_entry *dev_walk;

	major=dev>>8;
	minor=dev&0xff;

	/*
	 * BMAP_BOGUS_* are used in the Scyld userspace filesystem
	 * tool 'bush'.
	 */
	if(major==BMAP_BOGUS_MAJOR && minor==BMAP_BOGUS_MINOR)
	{
		return BMAP_BOGUS_FILENAME;
	} else {		
		dev_walk=bmap_dev_entries;
		while(dev_walk && dev_walk->filename)
		{
			if(dev_walk->major==major && dev_walk->minor==minor)
				return dev_walk->filename;
			dev_walk++;
		}
	}

	return NULL;
}

int
bogowipe(int fd,off_t offset,int len,unsigned char *block_buffer,int buffer_len)
{
int x;
int write_count;

	mft_log_entry();

	/* wipe with 0x0 */
	lseek(fd,offset,SEEK_SET);
	memset(block_buffer,0,buffer_len);
	for(x=0;x+buffer_len<len;x+=buffer_len)
	{
		write_count=write(fd,block_buffer,buffer_len);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}
	if(x<len)
	{
		write_count=write(fd,block_buffer,len-x);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}

	/* wipe with 0xff */
	lseek(fd,offset,SEEK_SET);
	memset(block_buffer,~0,buffer_len);
	for(x=0;x+buffer_len<len;x+=buffer_len)
	{
		write_count=write(fd,block_buffer,buffer_len);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}
	if(x<len)
	{
		write_count=write(fd,block_buffer,len-x);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}

	/* wipe with 0x0 */
	lseek(fd,offset,SEEK_SET);
	memset(block_buffer,0,buffer_len);
	for(x=0;x+buffer_len<len;x+=buffer_len)
	{
		write_count=write(fd,block_buffer,buffer_len);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}
	if(x<len)
	{
		write_count=write(fd,block_buffer,len-x);
		if(write_count<buffer_len)
		{
			mft_logf(MLOG_ERROR,"write error");
		}
	}

	mft_log_exit();
	return 0;
}
