/* helper.c -- application helper functions for the
 *	Mcgruff Forensic Toolkit
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/time.h>
#include <ctype.h>

#include "mft.h"

int mft_display_man(FILE *f,const char *date,int section,struct mft_info *info,...);
int mft_display_help(FILE *f,struct mft_info *info,...);
int mft_display_sgml(FILE *f,struct mft_info *info,...);
int mft_display_version(FILE *f,struct mft_info *info);

int
mft_display_man(FILE *f,const char *date,int section,struct mft_info *info,...)
{
va_list ap;
struct mft_option *option_walk;
struct mft_info *info_walk;
char **enum_walk;
struct mft_venum *venum_walk;

char *category="Brazil";
char *uppercase_progname;
char *string_walk;
int has_shortcuts=0;

	va_start(ap,info);

	if(!info)
		return -1;

	if(!f)
		f=stdout;

	uppercase_progname=strdup(info->name);
	string_walk=uppercase_progname;
	while(string_walk && *string_walk)
	{
		*string_walk=toupper(*string_walk);	
		string_walk++;
	}

	fprintf(f,".TH %s \"%d\" \"%s\" \"%s\" \"%s\"\n",uppercase_progname,section,date,info->version,category);

	fprintf(f,".SH NAME\n%s \\- %s\n",info->name,info->desc);
	fprintf(f,".SH SYNOPSIS\n");
	fprintf(f,".B %s\n",info->name);
	fprintf(f,"[\\fIOPTION\\fR]...\n");
	fprintf(f,".SH DESCRIPTION\n\n");
	
	info_walk=info;
	while(info_walk)
	{
		option_walk=info_walk->options;
		while(option_walk && option_walk->name)
		{
			if(!(option_walk->type&MOF_HIDDEN)) {
				if(option_walk->type&MOF_SILENT)
					has_shortcuts=1;

				if(option_walk->type&MFT_OPTION_TYPE_FLAG)
				{
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_STRING) {
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR \\fIARG\\fR %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_INT) {
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR \\fIINT\\fR %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_FILENAME) {
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR \\fIFILENAME\\fR %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_ENUM) {
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR \\fIVALUE\\fR %s\n",option_walk->name,option_walk->desc);
					fprintf(f,"\\fIVALUE\\fR can be one of:\n");
 
					enum_walk=option_walk->defval.d_enum;
					if(enum_walk && *enum_walk)
					{
						fprintf(f,".TP\n\t\\fB%s\\fR",*enum_walk);
						enum_walk++;
					}
					while(enum_walk && *enum_walk)
					{
						fprintf(f," | \\fB%s\\fR",*enum_walk);
						enum_walk++;
					}
					fprintf(f,"\n");
					if(option_walk->type&MOF_SILENT)
					{
						fprintf(f,".TP\n\t\\fBSHORTHAND INVOKATION:\\fR\n");
						fprintf(f,"Any of the valid values for \\fB--%s\\fR can be supplied directly as options. For instance, \\fB--%s\\fR can be used in place of \\fB--%s=%s\\fR.\n",option_walk->name,*(option_walk->defval.d_enum),option_walk->name,*(option_walk->defval.d_enum));
					}
				} else if(option_walk->type&MFT_OPTION_TYPE_VENUM) {
					fprintf(f,".TP\n\\fB\\-\\-%s\\fR \\fIVALUE\\fR %s\n",option_walk->name,option_walk->desc);
					fprintf(f,"\\fIVALUE\\fR can be one of:\n");
					venum_walk=(struct mft_venum *)(option_walk->defval.d_venum);
					while(venum_walk && venum_walk->name)
					{
						fprintf(f,".TP\n\t\\fB%s\\fR %s\n",venum_walk->name,venum_walk->desc);
						venum_walk++;
					}

					if(option_walk->type&MOF_SILENT)
					{
						fprintf(f,".TP\n\t\\fBSHORTHAND INVOKATION:\\fR\n");
						fprintf(f,"Any of the valid values for \\fB--%s\\fR can be supplied directly as options. For instance, \\fB--%s\\fR can be used in place of \\fB--%s=%s\\fR.\n",option_walk->name,((struct mft_venum *)(option_walk->defval.d_venum))->name,option_walk->name,((struct mft_venum *)(option_walk->defval.d_venum))->name);
					}
				} else {
					fprintf(f,"--%s %s\n",option_walk->name,option_walk->desc);
				}
			}
			option_walk++;
		}
		info_walk=va_arg(ap,struct mft_info *);
	}

	fprintf(f,".SH REPORTING BUGS\n");
	fprintf(f,"Report bugs to %s.\n",info->author);

	free(uppercase_progname);
	return 0;
}

int
mft_display_help(FILE *f,struct mft_info *info,...)
{
va_list ap;
struct mft_option *option_walk;
struct mft_info *info_walk;
char **enum_walk;
struct mft_venum *venum_walk;

	va_start(ap,info);

	if(!info)
		return -1;

	if(!f)
		f=stdout;

	fprintf(f,"Usage: %s [OPTION]...",info->name);
	option_walk=info->options;
	while(option_walk && option_walk->name)
	{
		if(
			!(option_walk->type&MOF_HIDDEN) &&
			option_walk->type&MOF_SILENT &&
			option_walk->type&MOT_FILENAME)
		{
			fprintf(f," [<%s-filename>]",option_walk->name);
		}
		option_walk++;
	}
	fprintf(f,"\n%s\n\n",info->desc);

	info_walk=info;
	while(info_walk)
	{
		option_walk=info_walk->options;
		while(option_walk && option_walk->name)
		{
			if(!(option_walk->type&MOF_HIDDEN)) {
				if(option_walk->type&MFT_OPTION_TYPE_FLAG)
				{
					fprintf(f,"--%s	%s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_STRING) {
					fprintf(f,"--%s <arg> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_INT) {
					fprintf(f,"--%s <int> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_FILENAME) {
					fprintf(f,"--%s <filename> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_ENUM) {
					fprintf(f,"--%s <",option_walk->name);
					enum_walk=option_walk->defval.d_enum;
					if(enum_walk && *enum_walk)
					{
						fprintf(f,"%s",*enum_walk);
						enum_walk++;
					}
					while(enum_walk && *enum_walk)
					{
						fprintf(f," | %s",*enum_walk);
						enum_walk++;
					}
					fprintf(f,"> %s\n",option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_VENUM) {
					fprintf(f,"--%s VALUE\n",option_walk->name);
					fprintf(f,"  where VALUE is one of:\n");
					venum_walk=(struct mft_venum *)(option_walk->defval.d_venum);
					while(venum_walk && venum_walk->name)
					{
						fprintf(f,"  %s  %s\n",venum_walk->name,venum_walk->desc);
						venum_walk++;
					}
			
				} else {
					fprintf(f,"--%s %s\n",option_walk->name,option_walk->desc);
				}
			}
			option_walk++;
		}
		info_walk=va_arg(ap,struct mft_info *);
	}

	return 0;
}

int
mft_display_sgml(FILE *f,struct mft_info *info,...)
{
va_list ap;
struct mft_option *option_walk;
struct mft_info *info_walk;
char **enum_walk;
struct mft_venum *venum_walk;

	va_start(ap,info);

	if(!info)
		return -1;

	if(!f)
		f=stdout;

	fprintf(f,"<tt>%s</tt> invocation\n<p>\n",info->name);
	fprintf(f,"<tt>%s [&lt;OPTIONS&gt;]",info->name);

	option_walk=info->options;
	while(option_walk && option_walk->name)
	{
		if(
			!(option_walk->type&MOF_HIDDEN) &&
			option_walk->type&MOF_SILENT &&
			option_walk->type&MOT_FILENAME)
		{
			fprintf(f," [&lt;%s-filename&gt;]",option_walk->name);
		}
		option_walk++;
	}
	fprintf(f,"</tt>\n\n");

	fprintf(f,"Where <bf>OPTIONS</bf> may include any of:\n");
	fprintf(f,"<descrip>\n");
	info_walk=info;
	while(info_walk)
	{
		option_walk=info_walk->options;
		while(option_walk && option_walk->name)
		{
			if(!(option_walk->type&MOF_HIDDEN)) {
				if(option_walk->type&MFT_OPTION_TYPE_FLAG)
				{
					fprintf(f,"<tag>--%s</tag>	%s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_STRING) {
					fprintf(f,"<tag>--%s &lt;arg&gt;</tag> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_INT) {
					fprintf(f,"<tag>--%s &lt;int&gt;</tag> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_FILENAME) {
					fprintf(f,"<tag>--%s &lt;filename&gt;</tag> %s\n",option_walk->name,option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_ENUM) {
					fprintf(f,"<tag>--%s &lt;",option_walk->name);
					enum_walk=option_walk->defval.d_enum;
					if(enum_walk && *enum_walk)
					{
						fprintf(f,"%s",*enum_walk);
						enum_walk++;
					}
					while(enum_walk && *enum_walk)
					{
						fprintf(f," | %s",*enum_walk);
						enum_walk++;
					}
					fprintf(f,"&gt;</tag> %s\n",option_walk->desc);
				} else if(option_walk->type&MFT_OPTION_TYPE_VENUM) {
					fprintf(f,"<tag>--%s VALUE</tag>\n",option_walk->name);
					fprintf(f,"  where VALUE is one of:\n");
					fprintf(f,"<descrip>\n");
					venum_walk=(struct mft_venum *)(option_walk->defval.d_venum);
					while(venum_walk && venum_walk->name)
					{
						fprintf(f,"<tag>%s</tag>  %s\n",venum_walk->name,venum_walk->desc);
						venum_walk++;
					}
					fprintf(f,"</descrip>\n");
			
				} else {
					fprintf(f,"<tag>--%s</tag> %s\n",option_walk->name,option_walk->desc);
				}
			}
			option_walk++;
		}
		info_walk=va_arg(ap,struct mft_info *);
	}
	fprintf(f,"</descrip>\n");

	return 0;
}

int
mft_display_version(FILE *f,struct mft_info *info)
{
	if(!f)
		return -1;

	if(!info)
		return -1;

	fprintf(f,"%s:%s %s\n",info->name,info->version,info->author);
	return 0;
}
