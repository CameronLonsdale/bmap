/* slacker.c handy bulk slack examiner.
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

/*
 * helper functions
 */
static int directory_examiner(const char *path,struct slacker_ops *module,mode_t raw_mode,struct stat path_statval,int flags);

/*
 * flags for major operating modes
 */
enum operating_modes {SLACKER_CAPACITY,SLACKER_FILL,SLACKER_FROB,SLACKER_POUR,SLACKER_WIPE};
enum doc_modes {SLACKER_VERSION,SLACKER_HELP,SLACKER_MAN,SLACKER_SGML};
int flag_mode=SLACKER_CAPACITY;	/* what operation are we going to perform */
int flag_recursive=1;		/* recurse through subdirectories */

struct mft_option options[]={
	{"doc","autogenerate document ...",
		MOT_VENUM|MOF_SILENT,
		MO_VENUM_CAST{
			{"version","display version and exit",
				0,MO_INT_CAST(SLACKER_VERSION)
			},
			{"help","display options and exit",
				0,MO_INT_CAST(SLACKER_HELP)
			},
			{"man","display man and exit",
				MOF_HIDDEN,MO_INT_CAST(SLACKER_MAN)
			},
			{"sgml","generate SGML invokation info",
				MOF_HIDDEN,MO_INT_CAST(SLACKER_SGML)
			},
			{NULL,NULL,0,MO_CAST(NULL)}
		}
	},
	{"mode","operation to perform on files",
		MOT_VENUM|MOF_SILENT,
		MO_VENUM_CAST{
			{"capacity","measure slack capacity of path",
				0,MO_INT_CAST(SLACKER_CAPACITY)
			},
			{"fill","fill slack space with user data",
				0,MO_INT_CAST(SLACKER_FILL)
			},
			{"frob","fill slack space with random data",
				0,MO_INT_CAST(SLACKER_FROB)
			},
			{"pour","write out the contents of slack space",
				0,MO_INT_CAST(SLACKER_POUR)
			},
			{"wipe","clear the contents of slack space",
				0,MO_INT_CAST(SLACKER_WIPE)
			},
			{NULL,NULL,0,MO_CAST(NULL)}
		},
	},
	{"outfile","write output to ...",MOT_FILENAME,MO_CAST(NULL)},
	{"verbose","be verbose",MOT_FLAG,MO_CAST(NULL)},
	{"log-thresh","logging threshold ...",
		MOT_ENUM,MO_ENUM_CAST{"none","fatal","error","info","branch","progress","entryexit",NULL}},
	{"path","operate on ...",MOT_FILENAME|MOF_SILENT,MO_CAST(NULL)},
	{"recursive","descend into subdirectories",MOT_FLAG,MO_CAST(NULL)},
	{0,0,0,MO_CAST(NULL)}
};

static struct mft_info slacker_info={
	"slacker",
	"examine and manipulate the aggregate slack of a directory tree",
	AUTHOR,
	VERSION,
	options
};

int
main(int argc,char **argv)
{
char *pathname=NULL;
mode_t raw_file_mode;
char *frag_filename=NULL;
int log_thresh;
struct stat path_statval;
int retval=0;

struct slacker_ops *module;

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
			case SLACKER_VERSION:
				mft_display_version(stdout,&slacker_info);
				break;
			case SLACKER_HELP:
				mft_display_version(stdout,&slacker_info);
				mft_display_help(stdout,&slacker_info,NULL);
				break;
			case SLACKER_MAN:
				mft_display_man(stdout,BUILD_DATE,1,&slacker_info,NULL);
				break;
			case SLACKER_SGML:
				mft_display_sgml(stdout,&slacker_info,NULL);
				break;
			}
			mft_log_exit();
			exit(0);
			break;
		case 1: /* mode */
			flag_mode=option_arg.a_enum;
			break;
		case 2: /* outfile */
			frag_filename=strdup(option_arg.a_string);
			break;
		case 3: /* verbose */
			mft_log_set(MLOG_INFO);
			break;
		case 4: /* log-thresh */
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
		case 5: /* path */
			pathname=option_arg.a_filename;
			break;
		case 6: /* recursive */
			flag_recursive=option_arg.a_flag;
			break;
		case -EINVAL: /* an option we don't understand */
			/* is this a non-option arg */
			if(option_arg.a_invalid && *option_arg.a_invalid=='-')
			{
				mft_logf(MLOG_FATAL,"invalid option: %s",option_arg.a_invalid);
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
	 * slacker requires a path.
	 */
	if(optval==-ENOENT || optval!=-EINVAL)
	{
		mft_log(MLOG_FATAL,"no path.");
		mft_log_exit();
		exit(2);
	} else {
		pathname=option_arg.a_invalid;
	}

	mft_logf(MLOG_PROGRESS,"target pathname: %s",pathname);

	raw_file_mode=0;
	module=NULL;
	if(flag_mode==SLACKER_CAPACITY)
	{
		module=&capacity_SYM;
		raw_file_mode=O_RDONLY;
	} else if(flag_mode==SLACKER_FILL) {
		module=&fill_SYM;
		raw_file_mode=O_RDWR;
	} else if(flag_mode==SLACKER_FROB) {
		module=&frob_SYM;
		raw_file_mode=O_RDWR;
	} else if(flag_mode==SLACKER_POUR) {
		module=&pour_SYM;
		raw_file_mode=O_RDONLY;
	} else if(flag_mode==SLACKER_WIPE) {
		module=&wipe_SYM;
		raw_file_mode=O_RDWR;
	}

	if(!module)
	{
		mft_logf(MLOG_FATAL,"Unknown mode (%d)",flag_mode);
		mft_log_exit();
		exit(44);
	}

	if(lstat(pathname,&path_statval)==-1)
	{
		mft_logf(MLOG_FATAL,"Unable to stat path: %s",pathname);
		mft_log_exit();
		exit(3);
	}

	/* we only do slacker on directories */
	if(!S_ISDIR(path_statval.st_mode))
	{
		mft_logf(MLOG_FATAL,"%s is not a directory.",pathname);
		mft_log_exit();
		exit(4);
	}

	retval=directory_examiner(pathname,module,raw_file_mode,path_statval,flag_recursive);

	module->cleanup();

	return retval;
}

static int
directory_examiner(const char *path,struct slacker_ops *module,mode_t raw_mode,struct stat path_statval,int flags)
{
DIR *target_dir;
struct dirent *target_dirent;
int dirpathlen;
char *pathbuf;
int target_fd=0;	/* fd of target file */
struct stat target_statval;		/* stat against target file */
int raw_fd;
unsigned char *block_buffer;
int spank_viable;
int retval;

	mft_log_entry();

	if(!module)
	{
		mft_logf(MLOG_ERROR,"no module");
		mft_log_exit();
		return -1;
	}

	if(!(target_dir=opendir(path)))
	{
		mft_logf(MLOG_ERROR,"couldn't open directory %s",path);
		mft_log_exit();
		return -1;
	}

	spank_viable=1;
	dirpathlen=strlen(path);
	while(spank_viable && (target_dirent=readdir(target_dir))!=NULL)
	{
	long slack_block;
	long slack_bytes;
	long block_size;

		/* we don't want "." or ".." -- by definition */
		if(strcmp(target_dirent->d_name,".") && strcmp(target_dirent->d_name,".."))
		{
			/* build the path */
			pathbuf=(char *)malloc(dirpathlen+2+strlen(target_dirent->d_name));
			strcpy(pathbuf,path);
			strcat(pathbuf,"/");
			strcat(pathbuf,target_dirent->d_name);

			mft_logf(MLOG_INFO,"examining %s",pathbuf);
			if(lstat(pathbuf,&target_statval)==-1)
			{
				mft_logf(MLOG_ERROR,"Unable to stat path: %s\n",path);
				mft_log_exit();
				return -1;
			}

			/* recurse */
			if(S_ISDIR(target_statval.st_mode) && flags)
			{
				
				mft_logf(MLOG_PROGRESS,"%s is a directory.",path);
				retval=directory_examiner(pathbuf,module,raw_mode,target_statval,flags);
				if(retval==0)
					spank_viable=0;
			
			} else if(S_ISREG(target_statval.st_mode) && target_statval.st_size>0) {
				target_fd=open(pathbuf,O_RDONLY);
				if((raw_fd=bmap_raw_open(pathbuf,raw_mode))==-1)
				{
					if(!errno)
						mft_logf(MLOG_INFO,"%s: couldn't raw open",pathbuf);
					else
						mft_logf(MLOG_ERROR,"%s: couldn't raw open (%s)",pathbuf,strerror(errno));
				} else {
					if((retval=bmap_get_slack_block(target_fd,&slack_block,&slack_bytes,&block_size))==-1)
					{
						mft_logf(MLOG_ERROR,"%s: couldn't map slack block",pathbuf);
					} else {
						block_buffer=(unsigned char *)calloc(1,block_size);
	
						mft_logf(MLOG_INFO,"slack bytes: %ld",slack_bytes);

						retval=module->spank(target_fd,raw_fd,slack_block*block_size+(block_size-slack_bytes),slack_bytes,block_buffer);

						free(block_buffer);

						if(retval<slack_bytes)
						{
							mft_logf(MLOG_BRANCH,"retval<slack_bytes");
							spank_viable=0;
						}
					}
					bmap_raw_close(raw_fd);
				}

				close(target_fd);

				free(pathbuf);
			}
		}
	}	

	closedir(target_dir);

	mft_log_exit();
	return spank_viable;
}
