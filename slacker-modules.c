/* slacker-modules.c -- functions for slacker
 *
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10227 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 */

/* config.h can contain preprocessor directives controlling system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mft.h"

#include "config.h"
#include "bmap.h"
#include "slacker.h"

static int spank_capacity(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);
static int spank_fill(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);
static int spank_frob(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);
static int spank_pour(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);
static int spank_wipe(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);

static void cleanup_capacity();
static void cleanup_fill();
static void cleanup_frob();
static void cleanup_pour();
static void cleanup_wipe();

struct slacker_ops capacity_SYM={
	spank_capacity,
	cleanup_capacity
};

struct slacker_ops fill_SYM={
	spank_fill,
	cleanup_fill
};

struct slacker_ops frob_SYM={
	spank_frob,
	cleanup_frob
};

struct slacker_ops pour_SYM={
	spank_pour,
	cleanup_pour
};

struct slacker_ops wipe_SYM={
	spank_wipe,
	cleanup_wipe
};

/*
 * modules follow...
 */

/*
 * see how big slack is
 */
static long long unformatted_capacity=0;
static long long unformatted_free=0;
static long long formatted_capacity=0;
static long long formatted_free=0;

static int
spank_capacity(int fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer)
{
unsigned char *zero_buffer;
int len;

	mft_log_entry();
	
	unformatted_capacity+=bytes;
	if(bytes>sizeof(struct slacker_record))
		formatted_capacity+=bytes-sizeof(struct slacker_record);
	else {
		mft_log_exit();
		return bytes;
	}

	zero_buffer=(unsigned char *)calloc(1,bytes);

	if(lseek(raw_fd,offset,SEEK_SET)!=offset)
	{
		mft_logf(MLOG_ERROR,"seek error");
		mft_log_exit();
		return -1;
	}

	len=read(raw_fd,block_buffer,bytes);	
	if(len<bytes)
	{
		mft_logf(MLOG_ERROR,"short read from slack");
	}

	if(!memcmp(block_buffer,zero_buffer,bytes))
	{
		unformatted_free+=bytes;
		formatted_free+=bytes-sizeof(struct slacker_record);	
	}

	free(zero_buffer);

	mft_log_exit();
	return bytes;
}

/*
 * put stuff into slack
 */
static int
spank_fill(int fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer)
{
int len;
int io_count;
int retval=bytes;

	mft_log_entry();

	if(lseek(raw_fd,offset,SEEK_SET)!=offset)
	{
		mft_logf(MLOG_ERROR,"seek error");
		mft_log_exit();
		return -1;
	}

	io_count=0;
	while(io_count<bytes)
	{
		len=read(0,block_buffer+io_count,bytes-io_count);	
		if(len==-1)
		{
			mft_logf(MLOG_ERROR,"read error on stdin");
			mft_log_exit();
			return -1;
		} else if(len==0) {
			mft_logf(MLOG_INFO,"EOF on stdin");
			break;
		} else {
			io_count+=len;	
		}
	}

	retval=write(raw_fd,block_buffer,io_count);
	if(retval<io_count)
	{
		mft_logf(MLOG_ERROR,"short write to slack");
	}

	mft_log_exit();
	return retval;
}

/*
 * put junk into slack
 */
static int
spank_frob(int fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer)
{
int io_count;
int rval;
int retval;

	mft_log_entry();

	if(lseek(raw_fd,offset,SEEK_SET)!=offset)
	{
		mft_logf(MLOG_ERROR,"seek error");
		mft_log_exit();
		return -1;
	}

	io_count=0;

	while(io_count+sizeof(int)<bytes)
	{
		*(int *)(block_buffer+io_count)=rand();
		io_count+=sizeof(int);
	}

	if(io_count<bytes)
	{
		rval=rand();
		memcpy(block_buffer+io_count,&rval,bytes-io_count);
	}

	retval=write(raw_fd,block_buffer,bytes);
	if(retval<bytes)
	{
		mft_logf(MLOG_ERROR,"short write to slack");
	}

	mft_log_exit();
	return bytes;
}

/*
 * dump the contents of slack 
 */
static int
spank_pour(int fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer)
{
int len;

	mft_log_entry();

	if(lseek(raw_fd,offset,SEEK_SET)!=offset)
	{
		mft_logf(MLOG_ERROR,"seek error");
		mft_log_exit();
		return -1;
	}
	len=read(raw_fd,block_buffer,bytes);	
	if(len<bytes)
	{
		mft_logf(MLOG_ERROR,"short read from slack");
	}
	write(1,block_buffer,len);

	mft_log_exit();
	return bytes;
}

/*
 * empty the slack
 */
static int
spank_wipe(int fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer)
{
int len;

	mft_log_entry();

	if(lseek(raw_fd,offset,SEEK_SET)!=offset)
	{
		mft_logf(MLOG_ERROR,"seek error");
		mft_log_exit();
		return -1;
	}
	memset(block_buffer,0,bytes);
	len=write(raw_fd,block_buffer,bytes);
	if(len<bytes)
	{
		mft_logf(MLOG_ERROR,"short write to slack");
	}

	mft_log_exit();
	return bytes;
}

static void cleanup_capacity()
{
	mft_log_entry();

	printf("unformatted capacity: %Ld\n",unformatted_capacity);
	printf("formatted capacity: %Ld\n",formatted_capacity);
	printf("unformatted free: %Ld\n",unformatted_free);
	printf("formatted free: %Ld\n",formatted_free);

	mft_log_exit();
}

static void cleanup_fill()
{
	mft_log_entry();
	mft_log_exit();
}

static void cleanup_frob()
{
	mft_log_entry();
	mft_log_exit();
}

static void cleanup_pour()
{
	mft_log_entry();
	mft_log_exit();
}

static void cleanup_wipe()
{
	mft_log_entry();
	mft_log_exit();
}
