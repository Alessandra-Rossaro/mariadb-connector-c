/* Copyright (C) 2000 MySQL AB & MySQL Finland AB & TCX DataKonsult AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA */

#include "mysys_priv.h"
#include "mysys_err.h"
#include <errno.h>


	/* Write a chunk of bytes to a file */

uint ma_write(int Filedes, const unsigned char *Buffer, uint Count, myf MyFlags)
{
  uint writenbytes,errors;
  ulong written;
  DBUG_ENTER("ma_write");
  DBUG_PRINT("my",("Fd: %d  Buffer: %lx  Count: %d  MyFlags: %d",
		   Filedes, Buffer, Count, MyFlags));
  errors=0; written=0L;

  for (;;)
  {
    if ((writenbytes = (uint) write(Filedes, Buffer, Count)) == Count)
      break;
    if ((int) writenbytes != -1)
    {						/* Safeguard */
      written+=writenbytes;
      Buffer+=writenbytes;
      Count-=writenbytes;
    }
    g_errno=errno;
    DBUG_PRINT("error",("Write only %d bytes, error: %d",
			writenbytes,g_errno));
#ifndef NO_BACKGROUND
#ifdef THREAD
    if (ma_thread_var->abort)
      MyFlags&= ~ MY_WAIT_IF_FULL;		/* End if aborted by user */
#endif
    if (g_errno == ENOSPC && (MyFlags & MY_WAIT_IF_FULL) &&
	(uint) writenbytes != (uint) -1)
    {
      if (!(errors++ % MY_WAIT_GIVE_USER_A_MESSAGE))
	ma_error(EE_DISK_FULL,MYF(ME_BELL | ME_NOREFRESH),
		 ma_filename(Filedes));
      VOID(sleep(MY_WAIT_FOR_USER_TO_FIX_PANIC));
      continue;
    }
    if (!writenbytes)
    {
      /* We may come here on an interrupt or if the file quote is exeeded */
      if (g_errno == EINTR)
	continue;
      if (!errors++)				/* Retry once */
      {
	errno=EFBIG;				/* Assume this is the error */
	continue;
      }
    }
    else if ((uint) writenbytes != (uint) -1)
      continue;					/* Retry */
#endif
    if (MyFlags & (MY_NABP | MY_FNABP))
    {
      if (MyFlags & (MY_WME | MY_FAE | MY_FNABP))
      {
	ma_error(EE_WRITE, MYF(ME_BELL+ME_WAITTANG),
		 ma_filename(Filedes),g_errno);
      }
      DBUG_RETURN(MY_FILE_ERROR);		/* Error on read */
    }
    else
      break;					/* Return bytes written */
  }
  if (MyFlags & (MY_NABP | MY_FNABP))
    DBUG_RETURN(0);			/* Want only errors */
  DBUG_RETURN(writenbytes+written);
} /* ma_write */
