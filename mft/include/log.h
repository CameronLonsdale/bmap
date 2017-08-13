/* log.h -- logging helper functions
 *          for the Mcgruff Forensics Toolkit
 *
 * Maintained 2000 by Daniel Ridge in support of:
 *   Scyld Computing Corporation.
 *
 * The maintainer may be reached as newt@scyld.com or C/O
 *   Scyld Computing Corporation
 *   10277 Wincopin Circle, Suite 212
 *   Columbia, MD 21044
 *
 * Written 1999,2000 by Daniel Ridge in support of:
 *   Network and Advanced Technology Crime Division,
 *   Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 *
 */

#ifndef NEWT_LOG_H
#define NEWT_LOG_H

/* logging functions */
#define MLOG_NONE 0
#define MLOG_FATAL 1		/* app is going to die */
#define MLOG_ERROR 2		/* non-fatal program error */
#define MLOG_INFO 4		/* user interest. */
#define MLOG_BRANCH 8		/* went this way */
#define MLOG_PROGRESS 16	/* got this far */
#define MLOG_ENTRYEXIT 32	/* entered or exitied this thing */
#define MLOG_OUTRAGEOUS	64	/* this is really, really verbose! */
extern int mft_log_init(void);
extern int mft_log_shutdown();
extern int mft_logf(int log_level,const char *format,...);
extern int mft_log(int log_level,const char *message);
extern int mft_plogf(int log_level,const char *position,const char *format,...);
extern int mft_plog(int log_level,const char *position,const char *message);
extern int mft_log_set(int thresh);
extern int mft_log_push(int thresh);
extern int mft_log_pop();
extern void mft_log_entry_helper(char *function);
extern void mft_log_exit_helper(char *function);
#define mft_log_entry() mft_log_entry_helper(__FUNCTION__)
#define mft_log_exit() mft_log_exit_helper(__FUNCTION__)
#define mft_log_return(value) {mft_log_exit_helper(__FUNCTION__); return(value);}
#define mft_log_vreturn {mft_log_exit_helper(__FUNCTION__); return;}
extern int mft_log_wouldlog(int thresh);
extern void mft_log_perror(int log_level,int eno,const char *message);
extern char *mft_log_lastentry(void);

/* initial values */
#ifndef INITIAL_LOG_THRESH
#define INITIAL_LOG_THRESH MLOG_ERROR
#endif

#endif
