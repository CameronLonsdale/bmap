/* slacker.h
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

#ifndef NEWT_SLACKER_H
#define NEWT_SLACKER_H

struct slacker_record {
	int prev_ino;
	int next_ino;
};

struct slacker_ops {
	int (*spank)(int target_fd,int raw_fd,off_t offset,long bytes,unsigned char *block_buffer);
	void (*cleanup)();	
};

extern struct slacker_ops capacity_SYM;
extern struct slacker_ops fill_SYM;
extern struct slacker_ops frob_SYM;
extern struct slacker_ops pour_SYM;
extern struct slacker_ops wipe_SYM;

#endif
