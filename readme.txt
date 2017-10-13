SecureDelete
------------

Program to walk the directory tree (from a nominated starting point) and securely delete every file it finds, and remove directories.

The purpose of this is to remove files and/or directories in a way that they cannot be later recovered by someone armed with a delete-recovery or disk editing program.

You might, for example, use it to delete files when:

a) The files are highly confidential 
b) Your PC is being given to someone else (eg. at work, or by selling it) 
c) You don't want other people using software *you* bought 
d) The files contain commercially sensitive data (eg. source code)


The program does this by:

a) Walking the directory tree from a nominated starting point -- OR -- considering a single file name supplied on the command line 
b) Counting files and directories 
c) Displaying the total and asking if you want to proceed 
d) For each file, it then opens the file for writing, and writes seven passes over the top of the existing data in the file 
e) The first pass consists of all zero bits, the second pass all one bits, and the subsequent passes randomly generated bits. The purpose of this is to defeat people armed with low-level disk analysers that might conceivably reconstruct your data if it was only changed to zeroes or ones. 
f) After the seven passes, the file is then removed from the disk directory. 
g) After each directory is processed the directory itself is removed

WARNING 
-------

1. You should watch for error messages - if a file is in use, read-only, or has "permissions" that don't allow you to change or delete it, then the program cannot process that file. Look for error messages, and if you see them, fix the problem (eg. change file security) and then try again.

2. For hopefully obvious reasons, the files cannot be recovered after they are deleted. Please ensure that you are deleting the directory or file(s) that you want to delete. For example, deleting C:\ is probably a bad idea.

3. The program deletes all subdirectories below a nominated directory. Make sure nothing important is in a subdirectory.

USAGE
-----

On the command line, specify the file or directory you want to delete. You cannot use wildcards. You either delete a single file, or a single directory.


DISCLAIMER
----------

The author does not accept responsibility for any files you may accidentally delete with this program. Use it with care.


SUMMARY
-------

Author: Nick Gammon <nick@gammon.com.au>

Web: http://www.gammon.com.au

Date: 27th May 1998

Usage: securedelete <starting_point>

eg. securedelete c:\games 
    securedelete /help <---- shows help information

Copyright 1998 by Nick Gammon - Gammon Software Solutions.

This program may be freely distributed provided credit is given to the author. It may not be commercially distributed (ie. for a fee) without a written arrangement with the author.
