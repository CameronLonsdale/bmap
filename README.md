# Cameron's Note:
bmap source code from 2000 would build correctly on my 64-bit machine but there was a struct alignment issue. This was due to the implementation being 32-bit specific, so forcing gcc to compile for 32-bit arhictecture fixes this issue. Enjoy the program!

# Install
Run `make`

# bmap: A filesystem-independant file blockmap utility for Linux

 Maintained 2000 by Daniel Ridge in support of:
   Scyld Computing Corporation.

 The maintainer may be reached as newt@scyld.com or C/O
   Scyld Computing Corporation
   10227 Wincopin Circle, Suite 212
   Columbia, MD 21044

 Written 1998 by Daniel Ridge in support of:
   Computer Crime Division, Office of Inspector General,
   National Aeronautics and Space Administration.

 The author may no longer be reached at NASA.
 Please direct all inquiries to the maintainer.

 This code is licensed to you under the terms of the GPL.
 See the file COPYING in this distribution for the terms.

---

WARNING: This may spank your hard drive.

---

VERSION CHANGES

1.0.18: (08/13/2017) cameron.lonsdale@gmail.com
	* forced 32 bit build to fix struct alignment issue on 64 bit machines.

1.0.17: (4/14/2000) newt@scyld.com
	* removed archaic index.html
	* removed mft as an included component. The scyld packager
	  now auto-includes this when you ask for bmap. mft will
	  now be maintained and versioned seperately.
	* BUGFIX: casting error created problems on files located above 2gb.
	  to fix this in older copies, look for assignments to 'offset' and
	  cast the first argument as 'long long'

1.0.16: (4/11/2000) newt@scyld.com
	* maintenance release. No useful changes.

1.0.15: (4/03/2000) newt@scyld.com
	* improved SGML documentation

1.0.14: (3/25/2000) newt@scyld.com
	* cleanup patchlevel. Removed stale patches from CVS to reflect
	some new organization.

1.0.13: (3/24/2000) newt@scyld.com
	* released courtesy copy to FBI CART.

1.0.12: (3/22/2000) newt@scyld.com
	* released courtesy copy to DCFL with interim documentation improvements.

1.0.11: (3/15/2000) newt@scyld.com
	* released courtesy copy to State Department.

1.0.10: (3/6/2000) newt@scyld.com
	* man pages are now auto-generated. This is made possible
	  now through additions to the support libraries. 'bmap' and
	  'slacker' will generate man pages when run with '--man'.
	* added a new option flag -- 'MOF_HIDDEN' that allows an option
	  to exist without being displayed in help screens or man pages.
	* added a new mode option to bmap '--mode=checkfrag'. This
	  checks for fragmentation and returns 0 if the file is fragmented.
	* moved bmap option '--mode=fragment' to '--mode=frag'
	* spun the support library functions into the 'mft' directory.
	  (These are the common library routines that various forensic
	   tools share with mcgruff).

1.0.9: (3/5/2000) newt@scyld.com
	* integrated latest option processing code from mcgruff.
	  try 'bmap --help' to see the difference that the new
	  'verbose enum' has made to the readability of the
	  built-in documentation.

1.0.8: (3/4/2000) newt@scyld.com
	* Updated man pages. Built on PowerPC linux.

1.0.7: (3/3/2000) newt@scyld.com
	* 'slacker' added as a companion utility for bmap. This utility
           operates on the collective slack of a directory tree and
	   performs many of the same slack operations as bmap.

1.0.6: (2/28/2000) newt@scyld.com
	* BUGFIX: bmap modes 'wipe','wipeslack','putslack','slackbytes'
	  failed to correctly operate on zero-length files. In certian
	  cases, this tool may attempt to write to block 0 of a raw
	  device. This is very bad.
	* BUGFIX: stat sometimes lies about the number of blocks in the file.
	  bmap no longer trusts these counts.

1.0.5: (2/24/2000) newt@scyld.com
	* improved logging.

1.0.4: (2/24/2000) newt@scyld.com
	* added support for 'raw device' operations in Rick Niles'
	  userspace filesystem shell.

1.0.3: (2/23/2000) newt@scyld.com
	* modified logging code to try to get initial log thresh
	  from environment variable (MCGRUFF_LOG_THRESH).
	* modified option processing code to allow options like
	  '--carve' to be interpreted as '--mode=carve' to supply
	  backwards compatibility

1.0.2: (2/22/2000) newt@scyld.com
	* rearranged invokation slightly. '--carve','--wipe', etc now
	  are invoked as '--mode=carve','--mode=wipe', etc.
	* added 'putslack' mode to write into slack.
	* added 'checkslack' mode to quietly test for slack.

1.0.1: (2/15/2000) newt@scyld.com
	* now maintained by Scyld Computing Corporation
	* added option and logging code from mcgruff

1.0.0: (12/28/1999) newt@hq.nasa.gov
	* promoted version to 1.0.

0.1.10: (12/22/1999) jakers@hq.nasa.gov
	* added 'label' option to print the physical sector information
	  on slack space
	* added 'fragfile' option to print fragmented file info to the 
	  filename specified. Enables a file to be sector mapped and 
	  highlighted if it is fragmented on same pass.
	* added 'name' option to print the name of the current file
	  being bmapped to stdout
	* added 'verbose' option to print status info on execution

0.1.9: (12/2/1999) newt@hq.nasa.gov
	* minor tweaks.

0.1.8: (11/12/1999) newt@hq.nasa.gov
	* bmap can now automatically find the device to slack and
	  carve from.

0.1.7: (11/12/1999) newt@hq.nasa.gov
	* Trailing whitespace in Makefile caused 'make install' to fail.
	* man pages were not being installed. (after all the trouble I went
	  to to write them!)

0.1.6: (9/2/1999) newt@hq.nasa.gov
	* added LICENSE file with copyright, warranty, and license information.

0.1.5: (1/7/1999) newt@hq.nasa.gov
	* added cheesy b2s byte->sector converter because bash only
	  performs shell arithmetic on longs.

0.1.4: (1/7/1999) newt@hq.nasa.gov
	* altered bmap to output sector numbers instead of block numbers

0.1.3: (1/4/1999) newt@hq.nasa.gov
	* built for AlphaLinux
	* built for SparcLinux
	* added '--carve' to bmap to carve out blocks associated with a file.
	* added '--slack' to bmap to carve out trailing data in the terminal
	  block of a file.
	* added '--raw' to bmap to specify name of raw device to read for
	  '--carve' and '--slack'

0.1.2: (1/1/1999) newt@hq.nasa.gov
	* added skeleton for bogoseek() for seeking on large files.
	* modified bmap to use lstat() for statting filenames. This allows
	  us to easily collate the results of several runs without having to
	  uniq blocks double-counted through dereferencing symlinks
	* corrected block count calculation to match observed behavior of
	  ext2fs on the author's machine.
	* added crude hole detection. high-quality hole detection will be
	  difficult.

0.1.1: (12/31/1998) newt@hq.nasa.gov
	* initial release.
