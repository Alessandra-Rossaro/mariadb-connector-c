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

/* Not MT-SAFE */

#ifdef SAFEMALLOC			/* We don't need SAFEMALLOC here */
#undef SAFEMALLOC
#endif

#include "mysys_priv.h"
#include "ma_static.h"
#include "mysys_err.h"

	/* alloc for things we don't nead to free */
	/* No DBUG_ENTER... here to get smaller dbug-startup */

gptr ma_once_alloc(unsigned int Size, myf MyFlags)
{
  size_t get_size,max_left;
  gptr point;
  reg1 USED_MEM *next;
  reg2 USED_MEM **prev;

  Size= ALIGN_SIZE(Size);
  prev= &ma_once_root_block;
  max_left=0;
  for (next=ma_once_root_block ; next && next->left < Size ; next= next->next)
  {
    if (next->left > max_left)
      max_left=next->left;
    prev= &next->next;
  }
  if (! next)
  {						/* Time to alloc new block */
    get_size= Size+ALIGN_SIZE(sizeof(USED_MEM));
    if (max_left*4 < ma_once_extra && get_size < ma_once_extra)
      get_size=ma_once_extra;			/* Normal alloc */

    if ((next = (USED_MEM*) malloc(get_size)) == 0)
    {
      g_errno=errno;
      if (MyFlags & (MY_FAE+MY_WME))
	ma_error(EE_OUTOFMEMORY, MYF(ME_BELL+ME_WAITTANG),get_size);
      return((gptr) 0);
    }
    DBUG_PRINT("test",("ma_once_malloc %u byte malloced",get_size));
    next->next= 0;
    next->size= get_size;
    next->left= get_size-ALIGN_SIZE(sizeof(USED_MEM));
    *prev=next;
  }
  point= (gptr) ((char*) next+ (next->size-next->left));
  next->left-= Size;

  return(point);
} /* ma_once_alloc */


	/* deallocate everything used by ma_once_alloc */

void ma_once_free(void)
{
  reg1 USED_MEM *next,*old;
  DBUG_ENTER("ma_once_free");

  for (next=ma_once_root_block ; next ; )
  {
    old=next; next= next->next ;
    free((gptr) old);
  }
  ma_once_root_block=0;

  DBUG_VOID_RETURN;
} /* ma_once_free */
