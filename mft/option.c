/* option.c -- command line helper functions
 *             for the Mcgruff Forensic Toolkit
 * 
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 * 
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10227 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 * Written 1999,2000 by Daniel Ridge in support of:
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/time.h>

#include "mft.h"

int mft_getopt(int argc,char **argv,struct mft_option *options,int flags,int *index,void *arg);

/*
 * internal helper functions
 */
static int process_match(struct mft_option *option,union option_arg *option_arg,char *val);

/* int
 * mft_getopt(
 *	int argc,
 *	char **argv,
 *	struct mft_option *options,
 *	int flags,
 *	int *index,
 *	void *arg)
 *
 * RETURN VALUES:
 *	retval
 *		-ENOENT	no more options available for processing
 *		-EFAULT	empty pointer in argv
 *		-EINVAL	an argument not encoded in options
 *			or an enum target not encoded in options
 *		-EDOM no argument supplied to an option requiring one
 *	index
 *		integer argv[] index to next argument to be processed.
 *	arg
 *		a union option_arg that points to possible values
 *		associated with an option key.
 *
 */
int
mft_getopt(int argc,char **argv,struct mft_option *options,int flags,int *index,void *arg)
{
int retval=0;
char *key=NULL,*val=NULL;
int key_len=0;
struct mft_option *option_walk;
char **enum_walk;
struct mft_venum *venum_walk;
union option_arg *option_arg=(union option_arg *)arg;

	mft_log_entry();

	/* Can we do anything? */
	if(!index) {
		mft_log(MLOG_BRANCH,"no index");
		mft_log_exit();
		return -ENOENT;
	}

	/* eat the program name */
	if(*index==0) {
		*index=1;
	}

	/* Are we valid? */
	if(*index>=argc || *index<0)
	{
		mft_logf(MLOG_BRANCH,"invalid index %d",*index);
		mft_log_exit();
		return -ENOENT;
	}

	/* Is there an option? */
	key=argv[*index];	
	if(!key) {
		mft_logf(MLOG_BRANCH,"argv[%d] is NULL",*index);
		retval=-EFAULT;
		(*index)++;
	} else if(*key!='-') {
		mft_logf(MLOG_BRANCH,"argv[%d] (%s) is not an option",*index,key);
		(*index)++;

		/* see if any filename or url options accept
		 * the silent flag
		 */
		option_walk=options;
		while(option_walk->name)
		{
			if(option_walk->type&MFT_OPTION_TYPE_FILENAME || option_walk->type&MFT_OPTION_TYPE_URL)
			{
			mft_logf(MLOG_PROGRESS,"examining a filename or url!");
			}
			option_walk++;
		}

		option_arg->a_invalid=key;
		retval=-EINVAL;
	} else {
		/* Eat leading dashes */
		while(*key=='-') key++;
	}

	if(retval)
	{
		mft_log_exit();
		return retval;
	}

	mft_logf(MLOG_PROGRESS,"%s is a well-formed argument",key);	

	/* find extent of option */
	key_len=0;
	while(*(key+key_len) && *(key+key_len)!='=') 
		key_len++;

	/* was a value bundled with this key? */
	val=NULL;
	if(*(key+key_len)=='=')
	{
		val=key+key_len+1;
	}

	option_walk=options;
	while(option_walk->name)
	{
		mft_logf(MLOG_PROGRESS,"checking against %s",option_walk->name);
		if(!strncmp(option_walk->name,key,strlen(option_walk->name)) && (*(key+strlen(option_walk->name))==0 || *(key+strlen(option_walk->name))=='='))
		{
			if(!val && *index<argc-1 && *argv[(*index)+1]!='-')
			{
				val=argv[(*index)+1];
				(*index)++;
			}
			break;
		} else if(option_walk->type&MFT_OPTION_TYPE_FLAG) {
			/* The user may wish to use '--flag-foo' as a way to
			 * explicitly specify the value of '--foo'.
			 */
			if(!strncmp("flag-",key,5) && !strncmp(option_walk->name,key+5,strlen(option_walk->name)) && (*(key+5+strlen(option_walk->name))==0 || *(key+5+strlen(option_walk->name))=='='))
			{
				mft_logf(MLOG_PROGRESS,"flagized option invokation");
				if(!val && *index<argc-1 && *argv[(*index)+1]!='-')
				{
					val=argv[(*index)+1];
					(*index)++;
				}
				if(!val)
				{
					mft_log_exit();
					return -EDOM;
				}
				break;
			}
		} else if(option_walk->type&MFT_OPTION_TYPE_ENUM && option_walk->type&MFT_OPTION_FLAG_SILENT && !val) {
			mft_logf(MLOG_PROGRESS,"examining an enum!");
			fflush(stderr);
			enum_walk=option_walk->defval.d_enum;
			while(enum_walk && *enum_walk)
			{
				mft_logf(MLOG_PROGRESS,"checking against %s",*enum_walk);
				if(!strcmp(*enum_walk,key))
				{
					mft_logf(MLOG_PROGRESS,"matched against an enum val");
					val=key;
				}
				enum_walk++;
			}
			if(val)
				break;
		} else if(option_walk->type&MFT_OPTION_TYPE_VENUM && option_walk->type&MFT_OPTION_FLAG_SILENT && !val) {
			mft_logf(MLOG_PROGRESS,"examining a venum!");
			fflush(stderr);
			venum_walk=(struct mft_venum *)(option_walk->defval.d_venum);
			while(venum_walk && venum_walk->name)
			{
				mft_logf(MLOG_PROGRESS,"checking against %s",venum_walk->name);
				if(!strcmp(venum_walk->name,key))
				{
					mft_logf(MLOG_PROGRESS,"matched against an venum val");
					val=key;
				}
				venum_walk++;
			}
			if(val)
				break;
		}
			option_walk++;
	}

	if(option_walk->name)
	{
		mft_logf(MLOG_PROGRESS,"arg matches against %s",option_walk->name);	
		process_match(option_walk,option_arg,val);

		(*index)++;
	} else {
		option_arg->a_invalid=argv[*index];	
		(*index)++;
		retval=-EINVAL;
	}

	if(!retval)
		retval=option_walk-options;

	mft_log_exit();	
	return retval;
}

static int
process_match(struct mft_option *option,union option_arg *option_arg,char *val)
{
char **enum_walk;
struct mft_venum *venum_walk;

	mft_log_entry();

	if(option->type & MFT_OPTION_TYPE_FLAG)
	{
		/*
		 * in order to facilitate the writing of certian scripts,
		 * we would like to be able to accept 'true' and 'false'
		 * options for scripts.
		 */
		if(	!val ||
			!strcasecmp("t",val) ||
			!strcasecmp("true",val) ||
			!strcasecmp("y",val) ||
			!strcasecmp("yes",val) ||
			!strcasecmp("on",val) ||
			*val=='1')
		{
			if(option_arg)
				option_arg->a_flag=1;
		} else {
			if(option_arg)
				option_arg->a_flag=0;
		}
	} else if(option->type & MFT_OPTION_TYPE_STRING) {
		if(val)
		{
			if(option_arg)
				option_arg->a_string=val;
		} else {
			if(option_arg)
				option_arg->a_string=NULL;
		}
	} else if(option->type & MFT_OPTION_TYPE_INT) {
		if(val)
		{
			if(option_arg)
				option_arg->a_int=atoi(val);
		} else {
			if(option_arg)
				option_arg->a_int=0;
		}
	} else if(option->type & MFT_OPTION_TYPE_FILENAME) {
		if(val)
		{
			if(option_arg)
				option_arg->a_filename=val;
		} else {
			if(option_arg)
				option_arg->a_filename=NULL;
		}
	} else if(option->type & MFT_OPTION_TYPE_ENUM) {
		if(val)
		{
			option_arg->a_enum=-1;	
			enum_walk=option->defval.d_enum;
			while(enum_walk && *enum_walk)
			{
				mft_logf(MLOG_PROGRESS,"checking against %s",*enum_walk);			
				if(!strcmp(*enum_walk,val))
				{
					mft_logf(MLOG_BRANCH,"matches against %s",*enum_walk);
					option_arg->a_enum=enum_walk-(option->defval.d_enum);
					break;
				}
				enum_walk++;
			}
			if(option_arg->a_enum==-1)
			{
				mft_logf(MLOG_ERROR,"invalid value for enum");
			}
		} else {
			option_arg->a_enum=-1;	
		}
	} else if(option->type & MFT_OPTION_TYPE_VENUM) {
		if(val)
		{
			option_arg->a_enum=-1;	
			venum_walk=(struct mft_venum *)(option->defval.d_venum);
			while(venum_walk && venum_walk->name)
			{
				mft_logf(MLOG_PROGRESS,"checking against %s",venum_walk->name);			
				if(!strcmp(venum_walk->name,val))
				{
					mft_logf(MLOG_BRANCH,"matches against %s",venum_walk->name);
					option_arg->a_enum=venum_walk->defval;
					break;
				}
				venum_walk++;
			}
			if(option_arg->a_enum==-1)
			{
				mft_logf(MLOG_ERROR,"invalid value for enum");
			}
		} else {
			option_arg->a_enum=-1;	
		}
	}

	mft_log_exit();
	return 0;
}
