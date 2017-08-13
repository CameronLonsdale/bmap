/* option.h -- command line argument helper functions
 *             for the Mcgruff Forensic Toolkit
 *
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10277 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 * Written 1998 by Daniel Ridge in support of:
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 */

#ifndef MFT_OPTION_H
#define MFT_OPTION_H

/* option types */
#define MFT_OPTION_TYPE_FLAG 1
#define MFT_OPTION_TYPE_STRING 2
#define MFT_OPTION_TYPE_INT	4
#define MFT_OPTION_TYPE_FILENAME 8
#define MFT_OPTION_TYPE_URL 8
#define MFT_OPTION_TYPE_ENUM 16
#define MFT_OPTION_TYPE_VENUM 32
#define MFT_OPTION_TYPE_MASK 0x111111	/* not a type */

/* The above type names are a little long */
#define MOT_FLAG MFT_OPTION_TYPE_FLAG	/* on/off yes/no true/false */
#define MOT_STRING MFT_OPTION_TYPE_STRING		/* "foo" */
#define MOT_INT	MFT_OPTION_TYPE_INT			/* 1,2,3 */
#define MOT_FILENAME MFT_OPTION_TYPE_FILENAME	/* "/foo/bar" */
#define MOT_URL MFT_OPTION_TYPE_URL		/* "http://www.foo.com/bar" */
#define MOT_ENUM MFT_OPTION_TYPE_ENUM	/* one of "up","down","left","right" */
#define MOT_VENUM MFT_OPTION_TYPE_VENUM	/* a more elaborate enum */

/* option flags */
#define MFT_OPTION_FLAG_SILENT 64
#define MFT_OPTION_FLAG_MULTI 128
#define MFT_OPTION_FLAG_HIDDEN 256
#define MFT_OPTION_FLAG_MASK 0x111000000	/* not a flag */

/* the above flag names are a little long */
#define MOF_SILENT MFT_OPTION_FLAG_SILENT	/* '--bar' -- '--foo=bar' */
#define MOF_MULTI MFT_OPTION_FLAG_MULTI	/* '--foo=bar, --foo=baz */
#define MOF_HIDDEN MFT_OPTION_FLAG_HIDDEN	/* '--easter-egg */

union mft_option_defval {
	void *d_null;
	int d_flag;
	char *d_string;
	int d_int;
	char *d_filename;
	char *d_url;
	char **d_enum;
	struct mft_option *d_venum;
};

struct mft_option {
	char *name;				/* name of the option */
	char *desc;				/* text blurb */
	int type;				/* the type of the argument */
	union mft_option_defval defval;		/* default value */
};

struct mft_venum {
	char *name;	/* name of enumerator */
	char *desc;	/* text blurb */
	int type;
	int defval;
};

#define MO_CAST (union mft_option_defval)
#define MO_INT_CAST (union mft_option_defval)(int)
#define MO_STRING_CAST (union mft_option_defval)(char *)
#define MO_ENUM_CAST (union mft_option_defval)(char **)(char *[])
#define MO_VENUM_CAST (union mft_option_defval)(struct mft_option *)(struct mft_option[])

#define MO_REMOVE 1

union option_arg {
	int a_flag;
	int a_int;
	char *a_string;
	int a_enum;
	char *a_filename;
	char *a_invalid;
};

extern int mft_getopt(int argc,char **argv,struct mft_option *options,int flags,int *index,void *arg);

#endif
