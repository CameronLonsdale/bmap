/* log.c -- logging helper functions
 *          for the Mcgruff Forensic Toolkit
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
 *   Computer Crime Division, Office of Inspector General,
 *   National Aeronautics and Space Administration.
 *
 * The author may be reached as newt@hq.nasa.gov or C/O
 *   NASA / Office of Inspector General
 *   300 E. St. SW, Washington DC 20546
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <syslog.h>

#include "mft.h"

int mft_log_init(void);
int mft_log_shutdown(void);
int mft_logf(int log_level,const char *format,...);
int mft_log(int log_level,const char *message);
int mft_plogf(int log_level,const char *position,const char *format,...);
int mft_plog(int log_level,const char *position,const char *message);
int mft_log_set(int thresh);
int mft_log_push(int thresh);
int mft_log_pop();
void mft_log_entry_helper(char *function);
void mft_log_exit_helper(char *function);
int mft_log_wouldlog(int thresh);

void mft_log_perror(int log_level,int eno,const char *message);
char *mft_log_lastentry(void);

static char *get_color(int log_level);

static int plain_dispatch(int log_level,const char *position,const char *message);
static int syslog_dispatch(int log_level,const char *position,const char *message);
static int html_dispatch(int log_level,const char *position,const char *message);
static int (*logger_dispatch)(int log_level,const char *position,const char *message)=plain_dispatch;

struct mft_log_settings {
	void *sp;				/* stack pointer */
	int thresh;				/* new threshold */
	struct mft_log_settings *next;	/* next entry */
};

/* sleazy globals */
struct mft_log_settings default_settings={0,MLOG_INFO,NULL};
struct mft_log_settings *log_settings=&default_settings;
static char *last_entry_name=NULL;

int
mft_log_init(void)
{
int retval=0;
char *enviro_thresh;
int enviro_level=0;

	mft_log_entry();

	/*
	 * For debugging purposes, we may wish to get our starting
	 * threshold out of the environment. We may be debugging the
	 * option processing code that's supposed to parse the option
	 * to set the logging threshold!
	 */
	if((enviro_thresh=getenv("MFT_LOG_THRESH")))
	{
		if(!strcasecmp("none",enviro_thresh))
			enviro_level=MLOG_NONE;
		else if(!strcasecmp("fatal",enviro_thresh))
			enviro_level=MLOG_FATAL;
		else if(!strcasecmp("error",enviro_thresh))
			enviro_level=MLOG_ERROR;
		else if(!strcasecmp("info",enviro_thresh))
			enviro_level=MLOG_INFO;
		else if(!strcasecmp("branch",enviro_thresh))
			enviro_level=MLOG_BRANCH;
		else if(!strcasecmp("progress",enviro_thresh))
			enviro_level=MLOG_PROGRESS;
		else if(!strcasecmp("entryexit",enviro_thresh))
			enviro_level=MLOG_ENTRYEXIT;
		else(enviro_thresh=NULL);	
	
		if(enviro_thresh)
			mft_log_set(enviro_level);	
	}
		
	mft_log_exit();
	return retval;
}

int
mft_log_shutdown(void)
{
	mft_log_entry();
	mft_log_exit();
	return 0;
}

int
mft_logf(int log_level,const char *format,...)
{
va_list ap;
char log_buf[1024]; 	/* NEWT: fix static allocation */

        va_start(ap,format);

	if(log_level>log_settings->thresh)
		return 0;

        if(format) 
        {
		strcpy(log_buf,"");
                vsnprintf(log_buf,sizeof(log_buf),format,ap);
		logger_dispatch(log_level,NULL,log_buf);
        }
        va_end(ap);
        return 0;
}

int
mft_log(int log_level,const char *message)
{
	if(log_level>log_settings->thresh)
		return 0;

	if(message)
		logger_dispatch(log_level,NULL,message);

	return 0;
}

int
mft_plogf(int log_level,const char *position,const char *format,...)
{
va_list ap;
char log_buf[1024]; 	/* NEWT: fix static allocation */

        va_start(ap,format);

	if(log_level>log_settings->thresh)
		return 0;

	if(format)
	{	
		strcpy(log_buf,"");
       	        vsnprintf(log_buf,sizeof(log_buf),format,ap);
	}


	if(position)
	{
		logger_dispatch(log_level,position,log_buf);
	} else {
		logger_dispatch(log_level,"unspecified",log_buf);
	}

        va_end(ap);
	return 0;
}

int
mft_plog(int log_level,const char *position,const char *message)
{
	if(log_level>log_settings->thresh)
		return 0;

	if(position)
	{
		logger_dispatch(log_level,position,message);
	} else {
		logger_dispatch(log_level,"unspecified",message);
	}

	return 0;
}

int
mft_log_set(int thresh)
{
struct mft_log_settings *old_settings,*new_settings;

	/* clear out the logging threshold stack */	
	old_settings=log_settings;

	/* the oldest entry on the stack is statically allocated.
	 * do not free.
	 */
	while(old_settings && old_settings->next)
	{
		new_settings=old_settings->next;

		free(old_settings);
		old_settings=new_settings;	
	}

	/* create a new entry */
	new_settings=(struct mft_log_settings *)calloc(1,sizeof(struct mft_log_settings));
	if(!new_settings)
		return -1;	/* should I log this? */

	new_settings->thresh=thresh;
	new_settings->next=old_settings;
	log_settings=new_settings;

	return 0;
}

int
mft_log_push(int thresh)
{
struct mft_log_settings *new_settings;

	new_settings=(struct mft_log_settings *)calloc(1,sizeof(struct mft_log_settings));
	if(!new_settings)
		return -1;	/* should I log this? */

	new_settings->thresh=thresh;
	new_settings->next=log_settings;
	log_settings=new_settings;

	return 0;
}

int
mft_log_pop()
{
struct mft_log_settings *old_settings;

	if(log_settings->next)
	{
		old_settings=log_settings;
		log_settings=old_settings->next;
		free(old_settings);
	}
	
	return 0;	
}

void
mft_log_entry_helper(char *function)
{
	last_entry_name=function;
	mft_plog(MLOG_ENTRYEXIT,function,"enter");
	return;
}

void
mft_log_exit_helper(char *function)
{
	mft_plog(MLOG_ENTRYEXIT,function,"exit");
	return;
}

int mft_log_wouldlog(int thresh)
{
	if(thresh>log_settings->thresh)
		return 0;
	else
		return 1;
}

void
mft_log_perror(int log_level,int eno,const char *message)
{
	if(eno<0) eno=-eno;
	if(eno>sys_nerr)
		return;
	
	mft_logf(log_level,"%s: %s",message,sys_errlist[eno]);
	return;
}

char *
mft_log_lastentry(void)
{
	return last_entry_name;
}

/*
 * internal helper functions
 */

static char *
get_color(int log_level)
{
static char color_buffer[1024]; /* NEWT fix static allocation */

	if(log_level&MLOG_FATAL)
		strcpy(color_buffer,"violet");
	else if(log_level&MLOG_ERROR)
		strcpy(color_buffer,"blue");
	else if(log_level&MLOG_INFO)
		strcpy(color_buffer,"green");
	else if(log_level&MLOG_BRANCH)
		strcpy(color_buffer,"yellow");
	else if(log_level&MLOG_PROGRESS)
		strcpy(color_buffer,"orange");
	else if(log_level&MLOG_ENTRYEXIT)
		strcpy(color_buffer,"red");
	else
		strcpy(color_buffer,"white");

	return color_buffer;
}

static int
plain_dispatch(int log_level,const char *position,const char *message)
{
	if(position)
	{
		if(message)
			fprintf(stderr,"%s: %s\n",position,message);
		else
			fprintf(stderr,"%s\n",position);
	} else {
		if(message)
			fprintf(stderr,"%s\n",message);
		else
			fprintf(stderr,"\n");
	}
	return 0;
}

static int
syslog_dispatch(int log_level,const char *position,const char *message)
{
        syslog(LOG_INFO,message);
	return 0;
}

static int
html_dispatch(int log_level,const char *position,const char *message)
{
	if(position)
	{
		if(message)
			fprintf(stderr,"<table bgcolor=%s><tr><td>%s: %s</td></tr></table><br>\n",get_color(log_level),position,message);
		else
			fprintf(stderr,"<table bgcolor=%s><tr><td>%s</td></tr></table><br>\n",get_color(log_level),position);
	} else {
		if(message)
			fprintf(stderr,"<table bgcolor=%s><tr><td>%s</td></tr></table><br>\n",get_color(log_level),message);
		else
			fprintf(stderr,"<table bgcolor=%s><tr><td></td></tr></table><br>\n",get_color(log_level));
	}

	return 0;
}
