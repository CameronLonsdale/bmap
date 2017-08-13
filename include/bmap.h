/* bmap.h -- header file for bmap tools
 *
 * Written 19^H^H2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The author may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10227 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 */

#ifndef NEWT_BMAP_H
#define NEWT_BMAP_H

#include <sys/stat.h>

/* bmap library functions */
extern int bmap_get_slack_block(
		int fd,
		long *slack_block,
		long *slack_bytes,
		long *block_size);
extern int bmap_get_block_size(int fd);
extern int bmap_get_block_count(
		int fd,
		const struct stat *statval);
extern int bmap_map_block(int fd,unsigned long block);
extern int bmap_raw_open(
		const char *filename,
		mode_t mode);
extern void bmap_raw_close(int fd);		

/********/

struct bmap_dev_entry {
	char *filename;
	int major;
	int minor;
};

extern struct bmap_dev_entry bmap_dev_entries[];
extern char *dev2filename(dev_t dev);
extern int bogowipe(int fd,off_t offset,int len,unsigned char *block_buffer,int buffer_len);

#endif
