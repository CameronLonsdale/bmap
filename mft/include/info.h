/* info.h
 *
 * Written 2000 by Daniel Ridge in support of:
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 */

#ifndef NEWT_INFO_H
#define NEWT_INFO_H

#include "option.h"

struct mft_info {
	char *name;		/* name of the client */
	char *desc;		/* text blurb */
	char *author;		/* text blurb */
	char *version;		/* text blurb */
	struct mft_option *options;
};

#endif
