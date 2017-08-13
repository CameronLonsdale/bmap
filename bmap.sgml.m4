<!doctype linuxdoc system>

<article>

<title>File System Block-Mapping under Linux
<author>Daniel Ridge, <tt>
   <htmlurl url="mailto:newt@scyld.com" name="newt@scyld.com"></tt>;
<date>v1.0.17, 16 March 2000

<abstract>
The Linux kernel includes a powerful, filesystem independant mechanism for
mapping logical files onto the sectors they occupy on disk. While this
interface is nominally available to allow the kernel to efficiently retrieve
disk pages for open files or running programs, an obscure user-space interface
does exist.

This is an interface which can be handily subverted (with <tt>bmap</tt> and
freinds) to perform a variety of functions interesting to the computer
forensics community, the computer security community, and the high-performance
computing community.
</abstract>

<sect>Downloading
<p>
bmap is publicly available at the following location
<itemize>
<item>Web page: <htmlurl url="http://www.scyld.com/software/bmap.html"
               name="http://www.scyld.com/software/bmap.html">
<item>Source: <htmlurl url="ftp://ftp.scyld.com/pub/bmap/bmap-1.0.17.tar.gz"
  name="ftp://ftp.scyld.com/pub/bmap/bmap-1.0.17.tar.gz">
</itemize>

<sect1>Redistribution
<p>
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
<p>
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
<p>
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

<sect>Usage

<p>The bmap package consists of 2 tools and a development library.
The standalone tools <tt>bmap</tt> and <tt>slacker</tt> are provided
as both useful standalone utilities and reference implementations of
<tt>libbmap</tt> applications.

<sect1>Building bmap 

<P><tt>make</tt> and <tt>make install</tt> should take care of it.

At this time, we have only worked out a bmap implementation on Linux.

These tools will install under <tt>/usr/local</tt> by default.

<sect1>Invoking the tools

<sect2>
include(bmap-invoke.sgml)

<sect2>
include(slacker-invoke.sgml)

<sect1>Limitations

<p>
The bmap works against filesystems mounted on block devices.
You will not to be able to operate against filesystems mounted
via Samba, NFS, or any other network filesystem.

If you simply cannot use bmap on the machine storing the block
device or block device image, you can try the linux network block driver
to export the block device to the machine from which you wish to bmap. Also,
Scyld's userfs user-level filesystem code includes a sample application,
bush, which is linked against bmap.

<sect1>Technical Description / Implementation

<p>

<sect2> VFS -- Linux 'Virtual Filesystem Switch'

<p>
These notes are based on Linux 2.2.5 but should be widely portable
to other versions of the Linux kernel.

compiled with
<verb>#include &lt;linux/fs.h&gt;</verb>

lives at <verb>((struct inode_operations *)foo)-&gt;bmap</verb>

and is prototyped as 
<verb>int foo_bmap(struct inode *inode,int block);</verb>

<sect2> FIBMAP -- userspace interface via ioctl

<p>
<verb>
#include &lt;sys/ioctl.h&gt;
#include &lt;linux/fs.h&gt;

retval=ioctl(fd,FIBMAP,&amp;block_pos);
</verb>

<p>Where <tt>block_pos</tt> passes the index of the block you wish to
map and returns the index of that block with respect to the underlying
block device. It is important to understand exactly what these arguments
expect and what they return:
<descrip>
	<tag>blocksize</tag>
		<tt>stat()</tt> is is happy to provide callers with a
		blocksize value. This blocksize is often not the right one
		for use with bmap. The <tt>stat()</tt> man page indicates
		that <tt>stat.st_blksize</tt> is for efficient filesystem I/O.
		The blocksize suited for use with bmap is available via 
		ioctl: <tt>ioctl(fd,FIGETBSZ,&amp;block_size)</tt> when
		performed against a file descriptor returns the file
		block size in bytes.
		
	<tag>index of the block you wish to map</tag>
		index is computed in units of blocksize per the
		above discussion. index is zero-based.

	<tag>offset of that block with respect to the underlying block device</tag>
		index is computed in units of blocksize per the above
		discussion. index is zero-based. <bf>NOTE:</bf> This offset
		is against the start of the block device on which the
		filesystem is mounted. This is usually a partition -- not
		the physical device on which the partition sits.
		Files with holes usually return 0 as their block offset for
		blocks that exist in the hole.
</descrip>

<sect2> Device Determination;

<p><tt>bmap</tt> and <tt>slacker</tt> contain code that allows them to 
do I/O against the raw block device. Under linux, it takes
a bit of work just to determine <bf>where</bf> a file is located.
<tt>stat()</tt> returns the major/minor of the block device via
<tt>stat.st_dev</tt> -- but this is difficult information to use.

Three ways leap immediately to mind:<descrip>
	<tag><tt>mknod</tt></tag>
		A new device node could be created somewhere with the
		major/minor numbers supplied by <tt>stat()</tt>. A
		serious downside is that a writeable volume must
		exist on the system in order for the device nodes to
		be created.

	<tag>walk /dev</tag>
		This method can be done with an existing filesystem, but
		the cost can be high. A /dev tree may feature thousands
		of entries on a modern system and the target entry may
		be buried hundreds or thousands of entries deep. This
		penalty could be extreme if the /dev tree were located
		on a remote system -- although this situation should be
		extremely rare.

	<tag>maintain an internal mapping</tag>
		This method is an attempt to speed up lookups in /dev
		by build-time precomputing a table with major/minor and
		node names for many block devices. The target device
		is checked to determine that the major/minor numbers
		are actually correct as a check.
</descrip>
<p>
<tt>bmap</tt> and freinds maintain an internal mapping for fast lookups.
This saves measureable time when bmap is invoked as the object of a file-system
walk over tens of thousands of files. Currently, however, they do not
search or store this mapping very efficiently.

<sect1>Advanced Block Map Techniques

<p>

<sect2>Undeleting files (brute force)
<p>
<enum>
	<item>Determine byte offset of string with respect to beginning of block
		device containing filesystem
	<item>Compute sector(s) containing string
	<item>Generate inode sector lists exhaustively over the filesystem
		<verb>find * -exec bmap {} &gt;&gt; /another_file_system/blocks \;</verb>
	<item>Sort lists from step (3) into a single list
		<verb>cat /another_file_system/blocks | sort -n | uniq &gt; > /another_file_system/blocks.sorted</verb>
	<item>Identify the contiguous set of unallocated sectors surrounding
		the sectors from step (4)
	<item>Extract the sector set identified in step (5)
	<item>Done
</enum>

<sect2>Undeleting files (openinode)
<p>
Scyld's <tt>openinode</tt> kernel patch relieves most of the complexity
of 'undeleting' files. However, a simple postprocessing step is often
useful when attempting to validate recovered files -- a check should
be made to determine if file blocks from the recovered file have been
subsequently allocated to other files.

<enum>
	<item>Generate inode sector list for the recovered file
	<item>Generate inode sector lists exhaustively over the filesystem
		<verb>find * -exec bmap {} &gt;&gt; /another_file_system/blocks \;</verb>
	<item>Sort lists from step (3) into a single list
		<verb>cat /another_file_system/blocks | sort -n | uniq &gt; > /another_file_system/blocks.sorted</verb>
	<item>See if any of the sectors reported for the recovered file
	<item>Done	
</enum>

Unfortunately, lack of collisions is not enough to guarantee that a
recovery is correct. Consider:

<enum>
	<item>User <tt>tom</tt> creates a file F(tom) containg the
	details of his baseball card collection. This results in the
	creation of an inode I(tom) mapped into the inode
	space of the filesystem and a vector of blocks V(tom)
	containing file data or metadata.

	<item>User <tt>tom</tt> deletes F(tom). Presuming that 
	no other links to I(tom) exist, the filesystem is now
	free to reclaim (seperately) both the inode entry I(tom)
	and the blocks listed in V(tom).

	<item>User <tt>dick</tt> creates a file F(dick) containing
	a great new picture of two midgets and a horse from
	alt.rec.stepladders.and.livestock. This results in the creation
	of an inode I(dick) mapped into the inode space of the
	filesystem and a vector of blocks V(dick) containing file
	data or metadata. Let us stipulate, for the example,
	that V(dick) exactly equals V(tom) -- which is to say
	that the picture of midgets now occupies the blocks
	previously dedicated to the baseball cards.

	<item>At this point, V(dick) may contain blocks reclaimed from
	V(tom). This does not imply that I(dick) is mapped into the
	filesystem on the same inode number as I(tom). We can detect
	this block reuse when recovering F(tom) by exhaustively
	comparing the elements of V(tom) against the elements of
	every other V() associated with every other I()
	in the filesystem -- we would learn that V(dick) contains blocks
	reclaimed from V(tom). Obviously, we must regard at least portions
	of F(tom) as unrecoverable if its blocks have been recycled!

	<item>User <tt>dick</tt> deletes F(dick). Presuming that 
	no other links to I(dick) exist, the filesystem is now
	free to reclaim (seperately) both the inode entry I(dick)
	and the blocks listed in V(dick).

	<item>At this point, a simple validation pass
	(as per above) would fail to reveal that V(tom) was
	reused as V(dick) because F(dick) has been removed.
	If we had failed to consider this point (as analysts
	surely have) we might have already fired <tt>tom</tt>
	from his job J(tom) for the midget picture! Perhaps we
	could increase the sophistication of the validation
	pass to survey every V() associated with every inode 
	in the inode space -- we could maybe see that
	a file,F(dick), was created after F(tom) and contained blocks
	reclaimed from V(tom).
	
	<item>The waters muddy further when user <tt>harry</tt> creates a
	file F(harry) containg his Christmas shopping list. This results
	in the creation of an inode I(harry) mapped into the inode
	space of the filesystem and a vector of blocks V(harry)
	containing file data or metadata. Let us stipulate, for the example,
	that I(harry) is mapped onto the same inode number that
	I(dick) was mapped onto. 

	<item>At this point, we are still tempted to believe that
	our recovered F(tom) contains a picture of midgets ;
	further that <tt>tom</tt> was deliberately hiding his
	pictures under a fake name. Unlike previous steps where
	a mechanism existed for determining that elements of V(tom)
	had been reallocated, every record of F(dick) -- namely I(dick) and
	V(dick) -- has been obliterated.
</enum>

While that situation sounds dire, there may still be hope for <tt>tom</tt>
before he's (wrongly) sent off to jail for child pornography. Modern
journalling filesystems may contain extra information that allows us
to exactly determine whether tom's original file is recoverable.
				
<sect1>Library Interface

<p>
<verb>
#include <bmap.h>
</verb>

<verb>
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
</verb>

<sect>Credits

<p>I would like to thank the <htmlurl url="www.hq.nasa.gov/office/oig/"
	name="NASA Office of Inspector General"> for having the special
needs that caused me to write this utility in the first place.

<p>I would like to thank Bob Hergert of the <htmlurl url="www.dcfl.gov"
	name="Defense Computer Forensics Lab"> for developing the
<tt>xscale</tt> companion utility and for testing this product.

<p>I would like to thank the FBI SWG-DE (Scientific Working Group on
Digital Evidence) for working to establish and promulgate guidelines
that make it feasable to apply high-performance computing techniques
to the computer forensics process.

</article>
