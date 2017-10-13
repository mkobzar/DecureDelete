#include "stdafx.h"
#include <io.h>
#include <errno.h>
#include <direct.h>

/*

  SecureDelete
  ------------

  Program to walk the directory tree (from a nominated starting point)
  and securely delete every file it finds, and remove directories.

  The purpose of this is to remove files and/or directories in a way that they
  cannot be later recovered by someone armed with a delete-recovery or
  disk editing program.

  You might, for example, use it to delete files when:

  a) The files are highly confidential
  b) Your PC is being given to someone else (eg. at work, or by selling it)
  c) You don't want other people using software *you* bought
  d) The files contain commercially sensitive data (eg. source code)


  The program does this by:

  a) Walking the directory tree from a nominated starting point
     -- OR -- considering a single file name supplied on the command line
  b) Counting files and directories
  c) Displaying the total and asking if you want to proceed
  d) For each file, it then opens the file for writing, and writes seven
     passes over the top of the existing data in the file (you can change
     the number of passes by changing the line below: #define PASSES 7 )
  e) The first pass consists of all zero bits, the second pass all one bits,
     and the subsequent passes randomly generated bits. The purpose of this
     is to defeat people armed with low-level disk analysers that might 
     conceivably reconstruct your data if it was only changed to zeroes or ones.
  f) After the seven passes, the file is then removed from the disk directory.
  g) After each directory is processed the directory itself is removed

  WARNING
  -------

  1. You should watch for error messages - if a file is in use, read-only, or has
     "permissions" that don't allow you to change or delete it, then the
     program cannot process that file. Look for error messages, and if you see
     them, fix the problem (eg. change file security) and then try again.

  2. For hopefully obvious reasons, the files cannot be recovered after they
     are deleted. Please ensure that you are deleting the directory or file(s)
     that you want to delete. For example, deleting C:\ is probably a bad idea.

  3. The program deletes all subdirectories below a nominated directory.
     Make sure nothing important is in a subdirectory.

  USAGE
  -----

  On the command line, specify the file or directory you want to delete. 
  You cannot use wildcards. You either delete a single file, or a single 
  directory.


  DISCLAIMER
  ----------

  The author does not accept responsibility for any files you may
  accidentally delete with this program. Use it with care.


  SUMMARY
  -------

  Author: Nick Gammon  <nick@gammon.com.au>

  Web:    http://www.gammon.com.au

  Date:   27th May 1998

  Usage:  securedelete <starting_point>

  eg.     securedelete c:\games
          securedelete /help     <---- shows help information

  Copyright 1998 by Nick Gammon - Gammon Software Solutions.

  This program may be freely distributed provided credit is given to the
  author. It may not be commercially distributed (ie. for a fee) without
  a written arrangement with the author.

*/


const char APPNAME [] = "SecureDelete";
long nErrors = 0;   // error count of delete errors
long nTotalFilesFound;   // count of total files and directories found

long nTotalFiles = 0;     // files found in this pass
long nTotalDirectories = 0; // directories found in this pass
__int64 nTotalSize = 0;     // total bytes in this pass

long nTotalFilesDeleted = 0;     // files deleted
long nTotalDirectoriesDeleted = 0; // directories deleted
__int64 nTotalSizeDeleted = 0;     // total bytes deleted

CStringList strCannotDeleteList;

#define PASSES 7    // number of passes of writing stuff onto the file

char buf [512];     // buffer for writing to disk

//===================================================================
// YesNo - prompts the user for a yes/no response
//===================================================================

bool YesNo (CString strMessage)
  {
  CString strReply;

  while (true)
    {
    printf ("%s", (LPCTSTR) strMessage);
    fgets  (buf, sizeof buf, stdin);
    strReply = buf;
    strReply.MakeUpper ();
    strReply.TrimLeft ();
    strReply.TrimRight ();
    if (strReply == "YES" || 
        strReply == "NO" ||
        strReply == "N" )
      break;
    printf ("Please reply YES or NO.\n");
    };
  return strReply == "YES";
  }

//===================================================================
// DeleteDirectory - deletes a specified directory
//===================================================================

void DeleteDirectory (CString strDirectoryName)
  {
  CString strError;

  printf  ("(%ld of %ld) Deleting directory: %s\n", 
                    nTotalFiles + nTotalDirectories, 
                    nTotalFilesFound,
                    (LPCTSTR) strDirectoryName);

  if (rmdir (strDirectoryName) == 0)
    {
    nTotalDirectoriesDeleted++;
    return;
    }

  perror (CFormat ("** Unable to delete %s", (LPCTSTR) strDirectoryName));

  nErrors++;

  strCannotDeleteList.AddTail (CFormat ("%s : %s",
                               (LPCTSTR) strDirectoryName, strerror (errno)));

  } // end of DeleteDirectory

//===================================================================
// DeleteFileSecurely - securely deletes a specified file
//===================================================================

void DeleteFileSecurely (CString & strFileName)
  {
  long filelength;

  printf ("    ");    // some leading spaces so strFileName stands out

  try
    {
    CFile f (strFileName , 
          CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary);

    filelength = f.GetLength ();

    for (int pass = 0; pass < PASSES; pass++)
      {

      printf ("*");   // a little "progress" star

  // prepare "buf" to have the right sort of data in it

      switch (pass)
        {
        case 0: memset (buf, 0, sizeof buf); break;     // pass 1
        case 1: memset (buf, 0xFF, sizeof buf); break;  // pass 2
        default:
          {                                             // other passes
          time_t timer;
          short a_rand;
          int i;

          time (&timer);
          srand (timer);  

  // fill the buffer with random data

          for (i = 0; i < (sizeof buf / sizeof a_rand); i++)
            {
            a_rand = rand ();
            memcpy (&buf [i * sizeof a_rand], &a_rand, sizeof a_rand);
            }

          }   // end of passes 3 to PASSES


        } // end of switch

      f.SeekToBegin ();   // back to start of file

      // write the data over the existing file

      for (long offset = 0; offset < filelength; offset += sizeof buf)
        {
        long bytes_to_write = filelength - offset;
  
        if (bytes_to_write > sizeof buf)
          bytes_to_write = sizeof buf;

        f.Write (buf, bytes_to_write);

        }   // end of scanning through the file

  // flush buffers to disk, to ensure that pattern is actually written

      f.Flush ();

      }   // end of each pass

    f.Close ();   // close file

    f.Remove (strFileName);    // and delete it

    printf (" --> deleted\n");  // tell the user we actually deleted it

    nTotalFilesDeleted++;
    nTotalSizeDeleted += filelength;

    }   // end of try block

  catch (CException * e)
    {

    e->GetErrorMessage (buf, sizeof (buf));
    printf ("** Error: %s\n", buf);
    e->Delete ();

    nErrors++;
    
    strCannotDeleteList.AddTail (buf);

    } // end of catch

  } // end of DeleteFileSecurely

//===================================================================
// ProcessDirectory - scan a directory - recurse for subdirectories
//===================================================================

void ProcessDirectory (CString dirname,       // root directory
                        CString subdirname,   // sub directory
                        int level,            // level of recursion
                        bool bDelete)         // true = delete 
  {

// if level is zero, see if this is a file rather than a directory

  if (level == 0)
    {
    CFileStatus rStatus;

    if (CFile::GetStatus (dirname, rStatus) &&
        (rStatus.m_attribute & _A_SUBDIR) == 0)
      { // found a file and it's is not a directory

      double mb = (double) rStatus.m_size / 1024.0 / 1024.0;
      nTotalSize += rStatus.m_size;
      nTotalFiles++;

      if (bDelete)    // delete pass - do the delete
        {
        printf ("(%ld of %ld) %9.3f Mb %s\n", 
                nTotalFiles + nTotalDirectories, 
                nTotalFilesFound,
                mb, (LPCTSTR) dirname);

        DeleteFileSecurely (dirname);

        }   // end of delete wanted
      else      // check pass - show the name
        printf ("%9.3f Mb %s\n", mb, (LPCTSTR) dirname);

      return;   // do not attempt to process as a directory
      }   // end of finding a file
    }   // end of level zero


// if we get here then the filename was not a directory

  CString thisdir = dirname;
  struct _finddata_t fileinfo;
  long hdl;

  // concatenate the subdirectory onto the root directory

  if (!subdirname.IsEmpty ())
    {
    thisdir += '\\';
    thisdir += subdirname;
    }

  // tell the user which directory we are processing

  printf ("Directory: %s\n", (LPCTSTR) thisdir);

  // set up a wildcard for directory scanning

  thisdir += "\\*.*";

  // start the scan

  hdl = _findfirst (thisdir, &fileinfo);

  // get rid of the wildcard - not needed now

  thisdir = thisdir.Left (thisdir.GetLength () - strlen ("\\*.*"));

  if (hdl == -1)
    {

    // no problem reported when reaching the end of a lower directory
    if (level != 0 && errno == ENOENT)
      return;

    // end of level zero - we didn't find a single thing!
    printf ("*** Unable to find any files named: %s\n", thisdir);    
    return;
    }

  // process entries in this directory until no more
  
  while (true)    
    {
    
    // if subdirectory - recurse to process it

    if (fileinfo.attrib & _A_SUBDIR)
      {
      if (strcmp (".", fileinfo.name) != 0 &&
          strcmp ("..", fileinfo.name) != 0)
        {
        ProcessDirectory (thisdir, 
                          fileinfo.name, 
                          level + 1, 
                          bDelete);
        nTotalDirectories++;
        if (bDelete)    // delete pass - delete the directory itself
          DeleteDirectory (CFormat ("%s\\%s", 
                            (LPCTSTR) thisdir, fileinfo.name));
        }   // end of not the special directories (. and ..)
      }   // end of subdirectory
    else  

    // just a file, count and optionally delete it

      {   // a file
      CString strFileName;

      strFileName = thisdir;
      strFileName += '\\';
      strFileName += fileinfo.name;
      double mb = (double) fileinfo.size / 1024.0 / 1024.0;
      nTotalSize += fileinfo.size;
      nTotalFiles++;

      if (bDelete)    // delete pass - do the delete
        {
        printf ("(%ld of %ld) %9.3f Mb %s\n", 
                nTotalFiles + nTotalDirectories, 
                nTotalFilesFound,
                mb, (LPCTSTR) strFileName);
        DeleteFileSecurely (strFileName);
        }   // end of delete wanted
      else      // check pass - show the name
        printf ("%9.3f Mb %s\n", mb, (LPCTSTR) strFileName);

      }   // end of having a file


    // get next directory entry
    if (_findnext (hdl, &fileinfo) == -1)
      break;    // out of loop

    };    // end of while loop - processing each directory entry

  _findclose (hdl);   // close directory handle

  // if we are scanning a directory, include *it* in the directory count

  if (level == 0)
    nTotalDirectories++;

  };    // end of ProcessDirectory

//===================================================================
// QueryUser - ask user if they are SURE they want to delete files
//===================================================================

bool QueryUser (long files, long directories, double mb)
  {
  printf (
   "***************************************************************\n");
  printf (
   "* Are you SURE you want to delete all of the above files?     *\n");
  printf (
   "* The files will be securely deleted and CANNOT be recovered. *\n");
  printf (
   "***************************************************************\n");

  if (!YesNo (CFormat ("OK to delete the above %ld file%s, %ld director%s? "
                       "Yes/No ... ",
            files,
            files == 1 ? "" : "s",
            directories,
            directories == 1 ? "y" : "ies"
      )))
    return false;

  if (!YesNo ( CFormat ("Are you ABSOLUTELY SURE you want to delete the \n"
            "  above %ld file%s, %ld director%s (%1.3f Mb)? Yes/No ... ", 
            files,
            files == 1 ? "" : "s",
            directories,
            directories == 1 ? "y" : "ies",
            mb
            )))
    return false;


  return true;
  } // end of QueryUser

//===================================================================
// main - program starts here
//===================================================================

int main (int argc, char * argv [])

  {

double mb;
char starting_point [_MAX_PATH] = "/?";   // default to showing help

  // I am using MFC things (eg. CString, exceptions) - so initialise it
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		fprintf (stderr, "MFC Failed to initialize.\n");
		return 1;
	}

  // check command-line options
  if (argc > 1)
    {
    strcpy (starting_point, argv [1]);

  // remove trailing backslash from starting point

    if (starting_point [strlen (starting_point) - 1] == '\\')
      starting_point [strlen (starting_point) - 1] = 0;

    } // end of having an argument

  printf ("[%s Version 1.00 Gammon Software Solutions]\n\n", APPNAME);

  if (strcmp (starting_point, "/?")     == 0 ||
      strcmp (starting_point, "/help")  == 0 ||
      strcmp (starting_point, "/HELP")  == 0 )
    {
    printf ("Usage: %s <file or directory>\n", APPNAME);
    printf ("\n");
    printf ("Written by Nick Gammon <nick@gammon.com.au>\n");
    printf ("Web: http://www.gammon.com.au\n");
    printf ("\n");
    return 0;
    }

  if (strchr (starting_point, '*') || strchr (starting_point, '?'))
    {
    printf ("You cannot use wildcards.\n");
    printf ("Either specify an individual file, or a directory.\n");
    return 1;
    } // end of testing for wildcards

  // set up a try-catch block, and get on with it

  try
    {

    // set to lower priority to make it easier to get rid of it
    if (SetThreadPriority(GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL) == 0)
                           ThrowSystemException ();
    
    ProcessDirectory (starting_point, "", 
                      0,    // level 0 - top level
                      false);   // don't delete

    mb = (double) nTotalSize / 1024.0 / 1024.0;
    printf ("\n");
    printf ("Found %ld file%s, %ld director%s (%1.3f Mb)\n", 
            nTotalFiles,
            nTotalFiles == 1 ? "" : "s",
            nTotalDirectories,
            nTotalDirectories == 1 ? "y" : "ies",
            mb);

    nTotalFilesFound = nTotalFiles + nTotalDirectories;

    // no files? give up

    if (nTotalFilesFound == 0)
      ThrowErrorException ("No files or directories found to delete");

    // ask user if they want to proceed with the deletes

    if (QueryUser (nTotalFiles, nTotalDirectories, mb))
      {
      nTotalSize = nTotalFiles = nTotalDirectories = 0;
      printf ("Deleting files and directories ...\n");
      ProcessDirectory (starting_point, "", 
                        0,    // level 0 - top level
                        true);    // delete this time

      // delete starting (root) directory

      if (nErrors == 0)   // can't delete directory if files still there
        DeleteDirectory (starting_point);

      // show deleted count

      mb = (double) nTotalSizeDeleted / 1024.0 / 1024.0;
      printf ("\n");
      printf ("Deleted %ld file%s, %ld director%s (%1.3f Mb)\n", 
              nTotalFilesDeleted,
              nTotalFilesDeleted == 1 ? "" : "s",
              nTotalDirectoriesDeleted,
              nTotalDirectoriesDeleted == 1 ? "y" : "ies",
              mb);
      if (nErrors)
        {
        printf ("** Unable to delete %ld file/director%s for reasons given above.\n",
                nErrors,
                nErrors == 1 ? "y" : "ies");

        if (YesNo ("** Show list of files/directories which could not be deleted? Yes/No ... "))
          do
            {
            for (POSITION pos = strCannotDeleteList.GetHeadPosition (); pos; )
              printf ("%s\n", (LPCTSTR) strCannotDeleteList.GetNext (pos));
            } while (YesNo ("** Show list again? Yes/No ... "));

        }
      }   // end of user wanting to do the deletes
    else
      printf ("Deletes not done.\n");

    } // end of try block

// catch any exceptions
  catch (CException * e)
    {

    e->GetErrorMessage (buf, sizeof (buf));
    printf ("** Error: %s\n", buf);
    e->Delete ();

    } // end of catch

  return 0;

  }
