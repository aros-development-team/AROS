/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <stdlib.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "private.h"

#include "NList_func.h"

LONG NL_GetSelects(struct NLData *data, LONG ent)
{
  LONG selects = 0L;

  if (data->multiselect && data->EntriesArray[ent]->Wrap && !(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
  {
    LONG bitpos = 1L,ent1 = ent + data->EntriesArray[ent]->dnum;
    BOOL sel = FALSE;

    while ((ent < data->NList_Entries) && (ent < ent1))
    {
      if (data->EntriesArray[ent]->Select != TE_Select_None)
      {
        selects |= bitpos;
        sel = TRUE;
      }
      else
        sel = FALSE;

      bitpos = bitpos << 1;
      ent++;
    }

    while (sel && (bitpos & 0x0000FFFF))
    {
      selects |= bitpos;
      bitpos = bitpos << 1;
    }

    if (sel)
      selects |= 0xFFFF8000;
    else
      selects &= 0x00007FFF;
  }

  return (selects);
}


//$$$ARRAY+
BOOL NL_InsertTmpLine(struct NLData *data,LONG pos)
{
  LONG newpos, ent, ent1, maxent;

  newpos = pos;
  maxent = data->NList_Entries + 1;

  if (maxent >= data->LastEntry)
  {
    struct TypeEntry **newentries = NULL;
    LONG le = ((maxent << 1) + 0x00FFL) & ~0x000FL;

    if((newentries = AllocVecPooled(data->Pool, sizeof(struct TypeEntry *) * (le + 1))) != NULL)
    {
      struct TypeEntry *newentry = NULL;

      if ((data->NList_First >= pos) && (data->NList_First < data->NList_Entries-1))
        data->NList_First++;

      data->LastEntry = le;
      ent = 0;
      ent1 = 0;

      { LONG de = newpos - ent1;
        if ((maxent - ent) < de)
          de = maxent - ent;
        NL_Move(&newentries[ent],&data->EntriesArray[ent1],de,ent);
        ent += de;
        ent1 += de;
      }

      if((newentry = AllocTypeEntry()) != NULL)
      {
        newentries[ent] = newentry;

        newentry->Entry = 0;
        newentry->Select = 0;
        newentry->Wrap = TE_Wrap_TmpLine;
        newentry->PixLen = -1;
        newentry->pos = 0;
        newentry->len = -2;
        newentry->style = 0;
        newentry->dnum = 1;
        newentry->entpos = ent;
        ent++;

        { LONG de = data->NList_Entries - ent1;
          if ((maxent - ent) < de)
            de = maxent - ent;
          NL_Move(&newentries[ent],&data->EntriesArray[ent1],de,ent);
          ent += de;
        }

        newentries[ent] = NULL;
        if (data->EntriesArray)
          FreeVecPooled(data->Pool, data->EntriesArray);
        data->EntriesArray = newentries;
        data->NList_Entries = ent;
        if ((newpos >= data->NList_AffFirst) || data->NList_EntryValueDependent)
        {
          NL_SegChanged(data,newpos,data->NList_Entries);
          data->do_draw = TRUE;
        }
        else
        {
          data->NList_AffFirst += 1;
          data->do_draw = TRUE;
        }
        if ((data->NList_Active >= newpos) && (data->NList_Active < data->NList_Entries))
        {
          set_Active(data->NList_Active + 1);
          NL_Changed(data,data->NList_Active);
        }
        UnSelectCharSel(data,FALSE);
        return (TRUE);
      }

      FreeVecPooled(data->Pool, newentries);
      return (FALSE);
    }
  }
  else if (data->EntriesArray)
  {
    struct TypeEntry *newentry = NULL;

    if ((data->NList_First >= pos) && (data->NList_First < data->NList_Entries-1))
      data->NList_First++;

    ent = data->NList_Entries;
    NL_MoveD(&data->EntriesArray[ent+1],&data->EntriesArray[ent],ent-newpos,ent+1);
    ent = newpos;

    if((newentry = AllocTypeEntry()) != NULL)
    {
      data->EntriesArray[ent] = newentry;

      newentry->Entry = 0;
      newentry->Select = 0;
      newentry->Wrap = TE_Wrap_TmpLine;
      newentry->PixLen = -1;
      newentry->pos = 0;
      newentry->len = -2;
      newentry->style = 0;
      newentry->dnum = 1;
      newentry->entpos = ent;
      data->NList_Entries += 1;

      if ((newpos >= data->NList_AffFirst) || data->NList_EntryValueDependent)
      {
        NL_SegChanged(data,newpos,data->NList_Entries);
        data->do_draw = TRUE;
      }
      else
      {
        data->NList_AffFirst += 1;
        data->do_draw = TRUE;
      }

      if ((data->NList_Active >= newpos) && (data->NList_Active < data->NList_Entries))
      {
        set_Active(data->NList_Active + 1);
        NL_Changed(data,data->NList_Active);
      }

      UnSelectCharSel(data,FALSE);

      return (TRUE);
    }
  }
  return (FALSE);
}


//$$$ARRAY+
void NL_DeleteTmpLine(struct NLData *data,LONG pos)
{
  LONG ent = pos;

  if((ent >= 0) && (ent < data->NList_Entries) && data->EntriesArray && (data->LastEntry > 0) &&
     (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
  {
    if((data->NList_First >= pos) && (data->NList_First > 0))
      data->NList_First--;
    data->NList_Entries -= 1;

    if((ent >= data->NList_AffFirst) || data->NList_EntryValueDependent)
    {
      NL_SegChanged(data,ent,data->NList_Entries + 1);
      data->do_draw = TRUE;
    }
    else
    {
      data->NList_AffFirst--;
      data->do_draw = TRUE;
    }

    if(data->NList_Active == ent)
    {
      DO_NOTIFY(NTF_Active | NTF_L_Active);
      if(data->NList_MultiSelect == MUIV_NList_MultiSelect_None)
      {
        if(ent+1 <= data->NList_Entries)
        {
          if(data->EntriesArray[ent+1]->Select == TE_Select_None)
            SELECT(ent+1,TE_Select_Line);
          data->lastselected = ent;
          data->lastactived = ent;
        }
        else if(ent-1 >= 0)
        {
          SELECT(ent-1,TE_Select_Line);
          data->lastselected = ent-1;
          data->lastactived = ent-1;
        }
      }
    }
    else if(data->NList_Active > ent)
    {
      set_Active(data->NList_Active - 1);
    }
    if(data->NList_Active >= data->NList_Entries)
    {
      set_Active(data->NList_Entries - 1);
    }

    FreeTypeEntry(data->EntriesArray[ent]);
    NL_Move(&data->EntriesArray[ent],&data->EntriesArray[ent+1],data->NList_Entries-ent,ent);
    ent = data->NList_Entries;

    data->EntriesArray[ent] = NULL;

    UnSelectCharSel(data,FALSE);
  }
}


struct sort_entry
{ LONG ent;
  struct NLData *data;
};


static int NL_SortCompar(struct NLData *data,LONG ent1,LONG ent2)
{
  LONG result = 0;
  LONG e1 = ent1;
  LONG e2 = ent2;

  if (data->EntriesArray[ent1]->Wrap & TE_Wrap_TmpLine)
    ent1 -= data->EntriesArray[ent1]->dnum;
  if (data->EntriesArray[ent2]->Wrap & TE_Wrap_TmpLine)
    ent2 -= data->EntriesArray[ent2]->dnum;

  if(ent1 != ent2)
  {
    result = (LONG)DoMethod(data->this, MUIM_NList_Compare, data->EntriesArray[ent1]->Entry, data->EntriesArray[ent2]->Entry, data->NList_SortType, data->NList_SortType2);
    if(result == 0)
      result = ent1 - ent2;
  }
  else
    result = e1 - e2;

  return (int)result;
}


/*int sort_compar(const void *e1, const void *e2)*/
static int sort_compar(struct sort_entry *e1, struct sort_entry *e2)
{
  return NL_SortCompar(e1->data,e1->ent,e2->ent);
}


//$$$ARRAY+
//fent = first entry
//lent = last entry
static ULONG NL_List_SortPart(struct NLData *data,LONG fent,LONG lent)
{
  LONG ent, ent1, numfollow, numexch, has_changed;
  struct sort_entry  *entry = NULL;
  struct TypeEntry *newentry;

  ENTER();

  data->display_ptr = NULL;
  has_changed = FALSE;

  lent++;

  if((entry = (struct sort_entry *)AllocVecPooled(data->Pool, sizeof(struct sort_entry) * (lent-fent))) != NULL)
  {
    for (ent = fent; ent < lent; ent++)
    {
      entry[ent-fent].ent = ent;
      entry[ent-fent].data = data;
    }

//D(DBF_ALWAYS, "qsort started.");
    qsort(entry, (size_t) (lent-fent), (size_t) sizeof(struct sort_entry), (int (*)()) &sort_compar);
//D(DBF_ALWAYS, "qsort ended.");

    ent1 = numfollow = numexch = 0;

    for (ent = fent; ent < lent; ent++)
    {
      ent1 = entry[ent-fent].ent;
      while (ent1 < ent)        /* find a ent1 not already done */
      {
        ent1 = entry[ent1-fent].ent;
        numfollow++;
        if(numfollow > lent) break;
      }

      if (ent1 > ent)           /* exchange them */
      {
        /* DO EXCHANGE */
        newentry = data->EntriesArray[ent];
        data->EntriesArray[ent] = data->EntriesArray[ent1];
        data->EntriesArray[ent1] = newentry;
        data->EntriesArray[ent]->entpos = ent;
        data->EntriesArray[ent1]->entpos = ent1;

        NL_Changed(data,ent);
        NL_Changed(data,ent1);

        if (data->NList_Active == ent)
          set_Active(ent1);
        else if (data->NList_Active == ent1)
          set_Active(ent);

        if (data->NList_LastInserted == ent)
        {
          /* sba: was data->NList_LastInserted--; */
          data->NList_LastInserted = ent1;
          DO_NOTIFY(NTF_Insert);
        } else
        if (data->NList_LastInserted == ent1)
        {
          /* sba: was data->NList_LastInserted++; */
          data->NList_LastInserted = ent;
          DO_NOTIFY(NTF_Insert);
        }
        has_changed = TRUE;
        data->moves = FALSE;

        entry[ent-fent].ent = ent1; /* redirect old current where it is now */
        numexch++;
      }
    }

//D(DBF_ALWAYS, "qsort stat: %ld exchanges, %ld follows.",numexch,numfollow);

    FreeVecPooled(data->Pool, entry);
  }

  RETURN(has_changed);
  return ((ULONG)has_changed);
}


ULONG NL_List_Sort(struct NLData *data)
{
  LONG has_changed;

  has_changed = NL_List_SortPart(data,0,data->NList_Entries-1);

  if (has_changed || data->do_wwrap)
  {
    data->sorted = TRUE;
    Make_Active_Visible;
    UnSelectCharSel(data,FALSE);
    if (has_changed)
    { DO_NOTIFY(NTF_Select | NTF_LV_Select);
    }
    if (data->do_wwrap)
      NL_DoWrapAll(data,TRUE,FALSE);
    data->do_updatesb = TRUE;
    REDRAW;
  }

/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/
  return ((ULONG)has_changed);
}


//$$$ARRAY+
static ULONG NL_List_SortMore(struct NLData *data,LONG newpos)
{
  LONG has_changed=0,fent,ment,lent;
  int comp;
  struct TypeEntry *newentry;

  if (newpos < data->NList_Entries-1)
    has_changed = NL_List_SortPart(data,newpos,data->NList_Entries-1);
  fent = 0;

  while (newpos < data->NList_Entries)
  {
    lent = newpos;
    while (fent < lent)
    { ment = (fent+lent)/2;
      comp = NL_SortCompar(data,newpos,ment);
      if (comp < 0)
        lent = ment;
      else
        fent = ment+1;
    }
    if (lent < newpos)
    { /* move  newpos  to  lent  */
      newentry = data->EntriesArray[newpos];

      NL_MoveD(&data->EntriesArray[newpos+1],&data->EntriesArray[newpos],newpos-lent,newpos+1);

      data->EntriesArray[lent] = newentry;
      data->EntriesArray[lent]->entpos = lent;


      if ((data->NList_Active >= lent) && (data->NList_Active < newpos))
      {
        set_Active(data->NList_Active + 1);
      }
      else if (data->NList_Active == newpos)
      {
        set_Active(lent);
      }
      NL_SegChanged(data,lent,newpos);
      data->NList_LastInserted = lent;
      DO_NOTIFY(NTF_Insert);
    }
    newpos++;
  }

  if (has_changed || data->do_wwrap)
  {
    data->sorted = TRUE;
    Make_Active_Visible;
    UnSelectCharSel(data,FALSE);
    if (has_changed)
    { DO_NOTIFY(NTF_Select | NTF_LV_Select);
    }
    if (data->do_wwrap)
      NL_DoWrapAll(data,TRUE,FALSE);
    data->do_updatesb = TRUE;
    REDRAW;
  }
  return ((ULONG)has_changed);
}


//$$$ARRAY+
ULONG NL_List_Insert(struct NLData *data,APTR *entries,LONG count,LONG pos,LONG wrapcol,LONG align,ULONG flags)
{
  BOOL success = FALSE;
  LONG newpos,count2;
  BOOL is_string = FALSE;
  char *string;

  STARTCLOCK(DBF_ALWAYS);
  wrapcol &= TE_Wrap_TmpMask;
  if (wrapcol)
  {
    LONG wrapcol2 = 1;

    while (wrapcol2 < TE_Wrap_TmpLine)
    {
      if (wrapcol & wrapcol2)
        break;
      wrapcol2 = wrapcol2 << 1;
    }
    wrapcol = wrapcol2 & TE_Wrap_TmpMask;
  }

  if (entries)
  {
    LONG ent, ent1, ent2, ent3, maxent, nlentries;

    //D(bug( "Inserting %ld entries at position %ld\n", count, pos ));

    data->display_ptr = NULL;
    if (count == -1)
    {
      count = 0;
      while (entries[count] != NULL)
        count++;
    }
    else if (count == -2)
    {
      is_string = TRUE;
      count = 1;
      string = (char *) entries;
      while (string[0] != '\0')
      {
        if ((string[0] == '\n') || (string[0] == '\r')) count++;
        if ((string[0] == '\r') && (string[1] == '\n')) string++;
        string++;
      }
      string = (char *) entries;
    }
    else if (count > 0)
    {
      newpos = 0;
      while (newpos < count)
      {
        if (entries[newpos] == NULL) count--;
        newpos++;
      }
    }

    if (count > 0)
    {
      maxent = count + data->NList_Entries;
      count2 = 0;

      switch (pos)
      {
        case MUIV_NList_Insert_Top:
          newpos = 0;
          break;
        case MUIV_NList_Insert_Bottom:
          newpos = data->NList_Entries;
          break;
        case MUIV_NList_Insert_Active:
          if ((data->NList_Active >= 0) && (data->NList_Active < data->NList_Entries))
            newpos = data->NList_Active;
          else
            newpos = data->NList_Entries;
          break;
        case MUIV_NList_Insert_Sorted:
        default:
          if ((pos >= 0) && (pos < data->NList_Entries))
            newpos = pos;
          else
            newpos = data->NList_Entries;
          break;
      }

      if ((newpos >= 0) && (newpos < data->NList_Entries) && (data->EntriesArray[newpos]->Wrap & TE_Wrap_TmpLine))
        newpos -= data->EntriesArray[newpos]->dnum;
      data->moves = FALSE;

      if (maxent >= data->LastEntry)
      {
        struct TypeEntry **newentries = NULL,**oldentries = data->EntriesArray;
        LONG le = ((maxent << 1) + 0x00FFL) & ~0x000FL;

        if((newentries = AllocVecPooled(data->Pool, sizeof(struct TypeEntry *) * (le + 1))) != NULL)
        {
          struct TypeEntry *newentry = NULL;

          //D(bug( "Allocating new entry array at 0x%08lx with %ld entries.\n", newentries, le+1 ));

          data->EntriesArray = newentries;
          nlentries = data->NList_Entries;
          data->NList_Entries += count;
          DO_NOTIFY(NTF_Entries|NTF_MinMax);
          data->LastEntry = le;
          ent = 0;
          ent1 = 0;
          ent3 = ent2 = 0;

          if (is_string)
            entries = (APTR *)(void*)&string;
          else
          {
            while (!entries[ent2])
              ent2++;
          }

          {
            LONG de = newpos - ent1;

            if ((maxent - ent) < de)
              de = maxent - ent;

            if(de != 0)
            {
              D(DBF_ALWAYS, "Moving %ld old entries to new array (from %ld to %ld)", de, ent1, ent);
              NL_Move(&data->EntriesArray[ent], &oldentries[ent1], de, ent);

              ent += de;
              ent1 += de;
            }
          }

          while ((ent < maxent) && (ent3 < count))
          {
            if((newentry = AllocTypeEntry()) != NULL)
            {
              data->EntriesArray[ent] = newentry;

              newentry->Entry = (APTR) DoMethod(data->this, MUIM_NList_Construct, entries[ent2], data->Pool);

              if (newentry->Entry)
              {
                newentry->Select = TE_Select_None;
                newentry->Wrap = wrapcol;
                newentry->PixLen = -1;
                newentry->pos = 0;
                newentry->len = -1;
                newentry->style = align;
                newentry->dnum = 1;
                newentry->entpos = ent;
                data->NList_LastInserted = ent;
                if(isFlagClear(flags, MUIV_NList_Insert_Flag_Raw))
                  NL_SetColsAdd(data,ent,FALSE);
                ent++;
              }
              else
                count2++;

              if (is_string)
              {
                while ((string[0] != '\0') && (string[0] != '\n') && (string[0] != '\r'))
                  string++;
                if ((string[0] == '\r') && (string[1] == '\n'))
                { string++; string++; }
                else if ((string[0] == '\n') ||(string[0] == '\r'))
                  string++;
                entries = (APTR *)(void*)&string;
              }
              else
              {
                ent2++;
                while (!entries[ent2])
                  ent2++;
              }
              ent3++;
            }
          }

          {
            LONG de = nlentries - ent1;

            if ((maxent - ent) < de)
              de = maxent - ent;

            if(de != 0)
            {
              D(DBF_ALWAYS, "Moving %ld old entries to new array (from %ld to %ld)", de, ent1, ent);
              NL_Move(&data->EntriesArray[ent], &oldentries[ent1], de, ent);

              ent += de;
            }
          }

          DO_NOTIFY(NTF_Insert);

          //D(bug( "Deleting pointer in entry %ld.\n", ent ));

          data->EntriesArray[ent] = NULL;
          if(oldentries != NULL)
            FreeVecPooled(data->Pool, oldentries);

          if(data->NList_Entries != ent)
          {
            DO_NOTIFY(NTF_Entries|NTF_MinMax);
          }

          data->NList_Entries = ent;
          count -= count2;

          /* TODO: This stuff here can be merged with the stuff below */
          GetNImage_End(data);
          GetNImage_Sizes(data);
          if (count > 0)
          {
            if ((newpos >= data->NList_AffFirst) || data->NList_EntryValueDependent)
              NL_SegChanged(data,newpos,data->NList_Entries);
            else
              data->NList_AffFirst += count;

            if ((data->NList_Active >= newpos) && (data->NList_Active < data->NList_Entries))
            {
              set_Active(data->NList_Active + count);
              NL_Changed(data,data->NList_Active);
            }

            UnSelectCharSel(data,FALSE);
            if (wrapcol)
              data->do_wwrap = TRUE;
            if (pos == MUIV_NList_Insert_Sorted)
            {
              BOOL needs_redraw;

              NL_SetCols(data);

              /* NL_List_SortXXX returns wheather sorting has changed anything
               * and redraws the entries. If sorting hasn't changed anything
               * we still have to redraw the list, because there are new entries
               * in the list */
              if (data->sorted)
                needs_redraw = !NL_List_SortMore(data,newpos);
              else
                needs_redraw = !NL_List_Sort(data);

              if (needs_redraw)
              {
                REDRAW;
              }
            }
            else
            {
              Make_Active_Visible;
              data->do_updatesb = TRUE;
              data->sorted = FALSE;
              REDRAW;
            }
/*            do_notifies(NTF_AllChanges|NTF_MinMax);*/
            success = TRUE;
          }
        }
      }
      else if (data->EntriesArray)
      {
        struct TypeEntry *newentry = NULL;

        ent = data->NList_Entries;
        nlentries = data->NList_Entries;
        data->NList_Entries += count;
        DO_NOTIFY(NTF_Entries|NTF_MinMax);

        D(DBF_ALWAYS, "Moving %ld entries (from %ld to %ld)", ent-newpos, ent, ent+count);
        NL_MoveD(&data->EntriesArray[ent+count], &data->EntriesArray[ent], ent-newpos, ent+count);

        ent = newpos;
        ent3 = ent2 = 0;

        if (is_string)
          entries = (APTR *)(void*)&string;
        else
        {
          while (!entries[ent2])
          ent2++;
        }

        while ((ent < data->LastEntry) && (ent3 < count))
        {
          if((newentry = AllocTypeEntry()) != NULL)
          {
            data->EntriesArray[ent] = newentry;

            newentry->Entry = (APTR) DoMethod(data->this, MUIM_NList_Construct, entries[ent2], data->Pool);

            if (newentry->Entry)
            {
              newentry->Select = TE_Select_None;
              newentry->Wrap = wrapcol;
              newentry->PixLen = -1;
              newentry->pos = 0;
              newentry->len = -1;
              newentry->style = align;
              newentry->dnum = 1;
              newentry->entpos = ent;
              data->NList_LastInserted = ent;
              if(isFlagClear(flags, MUIV_NList_Insert_Flag_Raw))
                NL_SetColsAdd(data,ent,FALSE);
              ent++;
            }
            else
              count2++;

            if (is_string)
            {
              while ((string[0] != '\0') && (string[0] != '\n') && (string[0] != '\r'))
                string++;
              if ((string[0] == '\r') && (string[1] == '\n'))
              { string++; string++; }
              else if ((string[0] == '\n') ||(string[0] == '\r'))
                string++;
              entries = (APTR *)(void*)&string;
            }
            else
            {
              ent2++;
              while (!entries[ent2])
                ent2++;
            }
            ent3++;
          }
        }

        data->NList_Entries -= count2;
        count -= count2;
        if (count2)
        {
          D(DBF_ALWAYS, "Moving %ld entries (from %ld to %ld)", data->NList_Entries-ent, ent+count2, ent);
          NL_Move(&data->EntriesArray[ent], &data->EntriesArray[ent+count2], data->NList_Entries-ent, ent);
        }

        DO_NOTIFY(NTF_Insert);

        GetNImage_End(data);
        GetNImage_Sizes(data);

        if (count > 0)
        {
          if ((newpos >= data->NList_AffFirst) || data->NList_EntryValueDependent)
            NL_SegChanged(data,newpos,data->NList_Entries);
          else
            data->NList_AffFirst += count;

          if ((data->NList_Active >= newpos) && (data->NList_Active < data->NList_Entries))
          {
            set_Active(data->NList_Active + count);
            NL_Changed(data,data->NList_Active);
          }
          UnSelectCharSel(data,FALSE);
          if (wrapcol)
            data->do_wwrap = TRUE;

          if (pos == MUIV_NList_Insert_Sorted)
          {
            int needs_redraw;

            NL_SetCols(data);

            /* NL_List_SortXXX returns wheather sorting has changed anything
             * and redraws the entries. If sorting hasn't changed anything
             * we still have to redraw the list, because there are new entries
             * in the list */
            if (data->sorted)
              needs_redraw = !NL_List_SortMore(data,newpos);
            else
              needs_redraw = !NL_List_Sort(data);

            if (needs_redraw)
            {
              REDRAW;
            }
          } else
          {
            Make_Active_Visible;
            data->do_updatesb = TRUE;
            data->sorted = FALSE;
            REDRAW;
          }
/*          do_notifies(NTF_AllChanges|NTF_MinMax);*/
          success = TRUE;
        }
      }
    }
  }
STOPCLOCK(DBF_ALWAYS, "insert");

  return success;
}


ULONG NL_List_Replace(struct NLData *data,APTR entry,LONG pos,LONG wrapcol,LONG align)
{
  ULONG result = FALSE;
  LONG ent;

  ENTER();

  wrapcol &= TE_Wrap_TmpMask;
  if (wrapcol != 0)
  {
    LONG wrapcol2 = 1;

    while(wrapcol2 < TE_Wrap_TmpLine)
    {
      if(wrapcol & wrapcol2)
        break;

      wrapcol2 = wrapcol2 << 1;
    }
    wrapcol = wrapcol2 & TE_Wrap_TmpMask;
  }

  switch (pos)
  {
    case MUIV_NList_Insert_Top:
    {
      ent = 0;
    }
    break;

    case MUIV_NList_Insert_Bottom:
    {
      ent = data->NList_Entries-1;
    }
    break;

    case MUIV_NList_Insert_Active:
    {
      // make sure there is an active element
      if(data->NList_Active >= 0 && data->NList_Active < data->NList_Entries)
      {
        ent = data->NList_Active;
      }
      else
      {
        // let the replacement fail if there is no active element
        ent = -1;
      }
    }
    break;

    case MUIV_NList_Insert_Sorted:
    default:
    {
      // make sure the position is valid
      if(pos >= 0 && pos < data->NList_Entries)
      {
        ent = pos;
      }
      else
      {
        // let the replacement fail if the position is invalid
        ent = -1;
      }
    }
    break;
  }

  SHOWVALUE(DBF_ALWAYS, ent);

  if(ent >= 0 && ent < data->NList_Entries && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    ent -= data->EntriesArray[ent]->dnum;

  if(entry != NULL && ent >= 0 && ent < data->NList_Entries)
  {
    // duplicate the given entry
    entry = (APTR)DoMethod(data->this, MUIM_NList_Construct, entry, data->Pool);

    if(entry != NULL)
    {
      data->display_ptr = NULL;
      NL_SetColsRem(data,ent);

      if(data->EntriesArray[ent]->Wrap && !(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine) && data->EntriesArray[ent]->len >= 0)
        data->EntriesArray[ent]->pos = (WORD) NL_GetSelects(data,ent);
      else
        data->EntriesArray[ent]->pos = 0;

      /* I don't understand this line but... ;) */
      if(!(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
        DoMethod(data->this, MUIM_NList_Destruct, data->EntriesArray[ent]->Entry, data->Pool);

      data->EntriesArray[ent]->Entry = entry;
      data->EntriesArray[ent]->Wrap = wrapcol;
      data->EntriesArray[ent]->PixLen = -1;
      data->EntriesArray[ent]->len = -1;
      data->EntriesArray[ent]->style = align;
      data->EntriesArray[ent]->dnum = 1;
      data->EntriesArray[ent]->entpos = ent;
      data->NList_LastInserted = ent;
      DO_NOTIFY(NTF_Insert);
      NL_SetColsAdd(data,ent,TRUE);
      NL_Changed(data,ent);
      UnSelectCharSel(data,FALSE);

      if(wrapcol != 0)
        data->do_wwrap = TRUE;

      data->do_updatesb = TRUE;
      data->sorted = FALSE;

      REDRAW;

      result = TRUE;
    }
    else
      result = NL_List_Remove(data,ent);
  }

  RETURN(result);
  return result;
}


//$$$ARRAY+
ULONG NL_List_Clear(struct NLData *data)
{
  LONG ent = data->NList_Entries - 1;

  STARTCLOCK(DBF_ALWAYS);
  DONE_NOTIFY(NTF_Select | NTF_LV_Select);
  data->display_ptr = NULL;
  NL_SetColsRem(data,-2);

  STARTCLOCK(DBF_ALWAYS);
  if(data->EntriesArray)
  {
    struct TypeEntry **entries = data->EntriesArray;
    BOOL notifySelect = FALSE;

    while(ent >= 0)
    {
      struct TypeEntry *entry = *entries;

      if(entry->Select != TE_Select_None)
        notifySelect = TRUE;

      if(isFlagClear(entry->Wrap, TE_Wrap_TmpLine))
        DoMethod(data->this, MUIM_NList_Destruct, entry->Entry, data->Pool);

      FreeTypeEntry(entry);
      ent--;
      entries++;
    }

    FreeVecPooled(data->Pool, data->EntriesArray);

    if(notifySelect == TRUE)
    {
      DO_NOTIFY(NTF_Select | NTF_LV_Select);
    }
  }
  STOPCLOCK(DBF_ALWAYS, "destruct");

  data->EntriesArray = NULL;
  data->LastEntry = 0;

  if(data->NList_Entries != 0)
  {
    DO_NOTIFY(NTF_Entries|NTF_MinMax);
    data->NList_Entries = 0;
  }

  if(data->NList_First != 0)
  {
    DO_NOTIFY(NTF_First);
    data->NList_First = 0;
  }

  set_Active(MUIV_NList_Active_Off);
  data->NList_Horiz_First = 0;
  data->NList_Visible = 0;
  data->NList_LastInserted = -1;
  data->Title_PixLen = -1;
  data->NList_DropMark = 0;
  data->sorted = FALSE;

  data->lastselected = MUIV_NList_Active_Off;
  data->lastactived = MUIV_NList_Active_Off;
  UnSelectCharSel(data,FALSE);
  NL_SetColsAdd(data,-1,TRUE);
  data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
  data->moves = FALSE;
  REDRAW_ALL;
/*  do_notifies(NTF_AllChanges|NTF_MinMax);*/
  STOPCLOCK(DBF_ALWAYS, "clear");

  return (TRUE);
}


//$$$ARRAY+
ULONG NL_List_Remove(struct NLData *data,LONG pos)
{
  LONG ent,ent2,skip,nlentries;
  if (pos == MUIV_NList_Remove_First)
    ent = 0;
  else if (pos == MUIV_NList_Remove_Last)
    ent = data->NList_Entries - 1;
  else if (pos == MUIV_NList_Remove_Active)
    ent = data->NList_Active;
  else if (pos == MUIV_NList_Remove_Selected)
  { ent2 = 0;
    while ((ent2 < data->NList_Entries) && (data->EntriesArray[ent2]->Select == TE_Select_None))
      ent2++;
    ent = ent2;
  }
  else
    ent = pos;
  if ((ent >= 0) && (ent < data->NList_Entries) && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    ent -= data->EntriesArray[ent]->dnum;
  if ((ent >= 0) && (ent < data->NList_Entries) && data->EntriesArray && (data->LastEntry > 0))
  {
    BYTE select;
    data->display_ptr = NULL;
    data->moves = FALSE;

    ent2 = ent + data->EntriesArray[ent]->dnum;

    if (data->NList_Entries <= 1)
      return NL_List_Clear(data);

    skip = FALSE;
    nlentries = data->NList_Entries;

    do
    { nlentries -= 1;

      if (data->EntriesArray[ent]->Wrap)
        data->do_wwrap = TRUE;

      NL_SetColsRem(data,ent);

      if (!(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
          DoMethod(data->this, MUIM_NList_Destruct, data->EntriesArray[ent]->Entry, data->Pool);

      DONE_NOTIFY(NTF_Select | NTF_LV_Select);

      select = data->EntriesArray[ent]->Select;
      if (data->EntriesArray[ent]->Select != TE_Select_None)
      {
        DO_NOTIFY(NTF_Select | NTF_LV_Select);
      }

      if ((ent >= data->NList_AffFirst) || data->NList_EntryValueDependent)
      {
        NL_SegChanged(data,ent,nlentries + 1);
      }
      else
        data->NList_AffFirst--;

      if (data->NList_Active == ent)
      {
        DO_NOTIFY(NTF_Active | NTF_L_Active);

        if (data->NList_MultiSelect == MUIV_NList_MultiSelect_None)
        {
          if (ent+1 <= nlentries)
          {
            if (data->EntriesArray[ent+1]->Select == TE_Select_None)
              skip = TRUE;
            SELECT(ent+1,TE_Select_Line);
            data->lastselected = ent;
            data->lastactived = ent;
          }
          else if (ent-1 >= 0)
          { SELECT(ent-1,TE_Select_Line);
            data->lastselected = ent-1;
            data->lastactived = ent-1;
          }
        }
        else
        {
          if ((ent+1 <= nlentries) &&
              (select == TE_Select_Line) &&
              (data->EntriesArray[ent+1]->Select == TE_Select_None))
          {
            if (data->EntriesArray[ent+1]->Select == TE_Select_None)
              skip = TRUE;
            SELECT(ent+1,TE_Select_Line);
            data->lastselected = ent;
            data->lastactived = ent;
          }
          else if ((ent-1 >= 0) &&
                   (select == TE_Select_Line) &&
                   (data->EntriesArray[ent-1]->Select == TE_Select_None))
          {
            SELECT(ent-1,TE_Select_Line);
            data->lastselected = ent-1;
            data->lastactived = ent-1;
          }
        }
      }
      else if (data->NList_Active > ent)
      {
        set_Active(data->NList_Active - 1);
      }
      if (data->NList_Active >= nlentries)
      {
        set_Active(nlentries - 1);
      }

      FreeTypeEntry(data->EntriesArray[ent]);

      NL_Move(&data->EntriesArray[ent],&data->EntriesArray[ent+1],nlentries-ent,ent);
      ent = nlentries;

      data->EntriesArray[ent] = NULL;

      if (pos == MUIV_NList_Remove_Selected)
      {
        if (skip)
          ent = ent2;
        else
          ent = ent2 - 1;
        skip = FALSE;
        while ((ent < nlentries) && (data->EntriesArray[ent]->Select == TE_Select_None))
          ent++;
        if ((ent >= 0) && (ent < nlentries))
        {
          if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
            ent -= data->EntriesArray[ent]->dnum;
          ent2 = ent + data->EntriesArray[ent]->dnum;
        }
      }
    } while ((pos == MUIV_NList_Remove_Selected) && (ent < nlentries));

    if (data->NList_Entries != nlentries)
    {
      DO_NOTIFY(NTF_Entries|NTF_MinMax);
    }

    data->NList_Entries = nlentries;
    if (data->NList_Entries <= 0)
      return NL_List_Clear(data);

/*
    if (data->NList_Entries < ((data->LastEntry/2) & ~0x000FL))
      maxent = (data->LastEntry/2) & ~0x000FL;

    if ((maxent > 0) && (newentries = AllocVecPooled(data->Pool, sizeof(struct TypeEntry *) * (maxent + 1))))
    { LONG ent1 = 0;
      data->LastEntry = maxent;
      maxent = 0;

      NL_Move(&newentries[ent1],&data->EntriesArray[ent1],data->NList_Entries-ent1,ent1);
      ent1 = data->NList_Entries;

      newentries[ent1] = NULL;
      if (data->EntriesArray)
        FreeVecPooled(data->Pool, data->EntriesArray);
      data->EntriesArray = newentries;
    }
*/

    if ((data->NList_First > 0) && (data->NList_First + data->NList_Visible >= data->NList_Entries))
    {
      data->NList_First = data->NList_Entries - data->NList_Visible;
      if (data->NList_First < 0)
        data->NList_First = 0;
      DO_NOTIFY(NTF_First);
    }
    UnSelectCharSel(data,FALSE);
    Make_Active_Visible;
    data->do_updatesb = TRUE;
    REDRAW;
/*    do_notifies(NTF_AllChanges|NTF_MinMax);*/

    return (TRUE);
  }
  return (FALSE);
}


ULONG NL_List_Exchange(struct NLData *data,LONG pos1,LONG pos2)
{
  LONG ent1, ent2;
  switch (pos1)
  {
    case MUIV_NList_Exchange_Top:
      ent1 = 0;
      break;
    case MUIV_NList_Exchange_Bottom:
      ent1 = data->NList_Entries - 1;
      break;
    case MUIV_NList_Exchange_Active:
      ent1 = data->NList_Active;
      break;
    default:
      ent1 = pos1;
      break;
  }
  if ((ent1 >= 0) && (ent1 < data->NList_Entries) && (data->EntriesArray[ent1]->Wrap & TE_Wrap_TmpLine))
    ent1 -= data->EntriesArray[ent1]->dnum;
  if ((ent1 >= 0) && (ent1 < data->NList_Entries))
  {
    switch (pos2)
    {
      case MUIV_NList_Exchange_Top:
        ent2 = 0;
        break;
      case MUIV_NList_Exchange_Bottom:
        ent2 = data->NList_Entries - 1;
        break;
      case MUIV_NList_Exchange_Active:
        ent2 = data->NList_Active;
        break;
      case MUIV_NList_Exchange_Next:
        ent2 = ent1 + 1;
        break;
      case MUIV_NList_Exchange_Previous:
        ent2 = ent1 - 1;
        break;
      default:
        ent2 = pos2;
        break;
    }
    if ((ent2 >= 0) && (ent2 < data->NList_Entries) && (data->EntriesArray[ent2]->Wrap & TE_Wrap_TmpLine))
      ent2 -= data->EntriesArray[ent2]->dnum;
    if ((ent2 >= 0) && (ent2 < data->NList_Entries) && (ent1 != ent2))
    {
      struct TypeEntry *newentry;

      data->display_ptr = NULL;

      newentry = data->EntriesArray[ent1];
      if (data->EntriesArray[ent1]->Wrap && !(data->EntriesArray[ent1]->Wrap & TE_Wrap_TmpLine) && (data->EntriesArray[ent1]->len >= 0))
      {
        newentry->pos = (WORD) NL_GetSelects(data,ent1);
        newentry->len = -1;
        data->do_wwrap = TRUE;
      }

      data->EntriesArray[ent1] = data->EntriesArray[ent2];
      data->EntriesArray[ent1]->entpos = ent1;
      if (data->EntriesArray[ent2]->Wrap && !(data->EntriesArray[ent2]->Wrap & TE_Wrap_TmpLine) && (data->EntriesArray[ent2]->len >= 0))
      {
        data->EntriesArray[ent1]->pos = (WORD) NL_GetSelects(data,ent2);
        data->EntriesArray[ent1]->len = -1;
        data->do_wwrap = TRUE;
      }

      data->EntriesArray[ent2] = newentry;
      data->EntriesArray[ent2]->entpos = ent2;

      if (data->NList_Active == ent1)
      {
        set_Active(ent2);
      }
      else if (data->NList_Active == ent2)
      {
        set_Active(ent1);
      }
      if ((ent1 >= data->NList_First) && (ent1 < data->NList_First+data->NList_Visible))
        NL_Changed(data,ent1);
      if ((ent2 >= data->NList_First) && (ent2 < data->NList_First+data->NList_Visible))
        NL_Changed(data,ent2);
      UnSelectCharSel(data,FALSE);
      Make_Active_Visible;
      NL_DoWrapAll(data,FALSE,FALSE);
      data->do_updatesb = TRUE;
      data->sorted = FALSE;
      REDRAW;
/*      do_notifies(NTF_AllChanges|NTF_MinMax);*/
      return (TRUE);
    }
  }
  return (FALSE);
}


//$$$ARRAY+
ULONG NL_List_Move_Selected(struct NLData *data,LONG to)
{
  LONG num_sel = 0;
  long first,last,ent,ent2,ent3,act,act2,dest;
  struct TypeEntry **EntriesArray;

  first = last = -1;
  if (!data->NList_TypeSelect)
  {
    ent = 0;
    while (ent < data->NList_Entries)
    {
      if (data->EntriesArray[ent]->Select != TE_Select_None)
      {
        if (data->EntriesArray[ent]->Wrap)
        {
          if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
            ent -= data->EntriesArray[ent]->dnum;
          if (data->EntriesArray[ent]->Wrap && (data->EntriesArray[ent]->len >= 0))
          {
            data->EntriesArray[ent]->pos = (WORD) NL_GetSelects(data,ent);
            data->EntriesArray[ent]->len = -1;
            data->do_wwrap = TRUE;
          }
          if (first == -1)
            first = ent;
          ent2 = ent;
          last = ent + data->EntriesArray[ent]->dnum;
          data->EntriesArray[ent]->Select = TE_Select_Line;
          num_sel++;
          ent++;
          while ((ent < data->NList_Entries) && (ent < last) && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
          {
            data->EntriesArray[ent]->Select = TE_Select_Line;
            data->EntriesArray[ent]->dnum = ent - ent2;
            num_sel++;
            ent++;
          }
          data->EntriesArray[ent2]->dnum = ent - ent2;
          last = ent - 1;
        }
        else
        {
          if (first == -1)
            first = ent;
          last = ent;
          num_sel++;
          ent++;
        }
      }
      else
        ent++;
    }
  }
  else
  {
    first = data->sel_pt[data->min_sel].ent;
    last = data->sel_pt[data->max_sel].ent;
    if ((data->sel_pt[data->max_sel].column == 0) && (data->sel_pt[data->max_sel].xoffset == PMIN))
      last--;
    if ((first >= 0) && (first < data->NList_Entries) && (data->EntriesArray[first]->Wrap & TE_Wrap_TmpLine))
      first -= data->EntriesArray[first]->dnum;
    if ((last >= 0) && (last < data->NList_Entries) && (data->EntriesArray[last]->Wrap & TE_Wrap_TmpLine))
    { last -= data->EntriesArray[last]->dnum;
      if ((last >= 0) && (last < data->NList_Entries))
        last += data->EntriesArray[last]->dnum - 1;
    }
    if ((first >= 0) && (first < data->NList_Entries) && (last >= 0) && (last < data->NList_Entries) && (first <= last))
      num_sel = last - first + 1;
  }
  if (num_sel <= 0)
    return (TRUE);
  else if (num_sel == 1)
    return NL_List_Move(data,last,to);
  else
  {
    switch (to)
    {
      case MUIV_NList_Move_Top:
        dest = 0;
        break;
      case MUIV_NList_Move_Bottom:
        dest = data->NList_Entries;
        break;
      case MUIV_NList_Move_Active:
        dest = data->NList_Active;
        break;
      default:
        dest = to;
        if ((dest < 0) || (dest > data->NList_Entries))
          dest = data->NList_Entries;
        break;
    }
    if ((dest >= 0) && (dest < data->NList_Entries) && (data->EntriesArray[dest]->Wrap & TE_Wrap_TmpLine))
      dest -= data->EntriesArray[dest]->dnum;

    if ((last-first+1 == num_sel) && (first <= dest) && (dest <= last+1))
    {
      if (data->do_wwrap)
      {
        NL_DoWrapAll(data,FALSE,FALSE);
        REDRAW;
      }
      return (TRUE);
    }
    else if((EntriesArray = (struct TypeEntry **)AllocVecPooled(data->Pool, sizeof(struct TypeEntry *)*num_sel)) != NULL)
    {
      data->display_ptr = NULL;
      ent = ent2 = first;
      ent3 = 0;
      act = data->NList_Active;
      act2 = MUIV_NList_Active_Off;

      while (ent < data->NList_Entries)
      {
        if ((!data->NList_TypeSelect && (data->EntriesArray[ent]->Select != TE_Select_None)) ||
            (data->NList_TypeSelect && (ent >= first) && (ent <= last)))
        {
          if (dest > ent2)
            dest--;
          if (data->NList_Active == ent)
            act2 = ent3;
          else if (act > ent2)
            act--;
          EntriesArray[ent3] = data->EntriesArray[ent];
          EntriesArray[ent3]->entpos = ent3;
          ent3++;
        }
        else
        {
          data->EntriesArray[ent2] = data->EntriesArray[ent];
          data->EntriesArray[ent2]->entpos = ent2;
          if (data->EntriesArray[ent]->Wrap && !(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine) && (data->EntriesArray[ent]->len >= 0))
          {
            data->EntriesArray[ent2]->pos = (WORD) NL_GetSelects(data,ent);
            data->EntriesArray[ent2]->len = -1;
            data->do_wwrap = TRUE;
          }
          ent2++;
        }
        ent++;
      }
      if (act2 >= 0)
      {
        set_Active(dest + act2);
      }
      else if (act >= dest)
      {
        set_Active(act + ent3);
      }
      else
      {
        set_Active(act);
      }
      data->NList_LastInserted = dest;
      DO_NOTIFY(NTF_Insert);
      ent2--;
      while (ent2 >= dest)
      {
        data->EntriesArray[ent2+ent3] = data->EntriesArray[ent2];
        data->EntriesArray[ent2+ent3]->entpos = ent2+ent3;
        if (data->EntriesArray[ent2]->Wrap && !(data->EntriesArray[ent2]->Wrap & TE_Wrap_TmpLine) && (data->EntriesArray[ent2]->len >= 0))
        {
          data->EntriesArray[ent2+ent3]->pos = (WORD) NL_GetSelects(data,ent2);
          data->EntriesArray[ent2+ent3]->len = -1;
          data->do_wwrap = TRUE;
        }
        ent2--;
      }
      ent2++;

      NL_Move(&data->EntriesArray[ent2],&EntriesArray[0],ent3,ent2);
      FreeVecPooled(data->Pool, EntriesArray);

      if (dest < first)
        first = dest;
      if (dest+ent3 > last)
        last = dest+ent3;

      NL_SegChanged(data,first,last);
      UnSelectCharSel(data,FALSE);
      Make_Active_Visible;
      NL_DoWrapAll(data,FALSE,FALSE);
      data->do_updatesb = TRUE;
      REDRAW;
/*      do_notifies(NTF_AllChanges|NTF_MinMax);*/
      return (TRUE);
    }
    else if (data->do_wwrap)
    {
      NL_DoWrapAll(data,FALSE,FALSE);
      REDRAW;
    }
  }
  return (FALSE);
}


//$$$ARRAY+
ULONG NL_List_Move(struct NLData *data,LONG from,LONG to)
{
  LONG ent1, ent2;

  if(from == MUIV_NList_Move_Selected)
    return NL_List_Move_Selected(data, to);

  switch(from)
  {
    case MUIV_NList_Move_Top:
      ent1 = 0;
    break;

    case MUIV_NList_Move_Bottom:
      ent1 = data->NList_Entries - 1;
    break;

    case MUIV_NList_Move_Active:
      ent1 = data->NList_Active;
    break;

    default:
      ent1 = from;
    break;
  }

  if(ent1 >= 0 && ent1 < data->NList_Entries && isFlagSet(data->EntriesArray[ent1]->Wrap, TE_Wrap_TmpLine))
    ent1 -= data->EntriesArray[ent1]->dnum;

  if(ent1 >= 0 && ent1 < data->NList_Entries)
  {
    switch(to)
    {
      case MUIV_NList_Move_Top:
        ent2 = 0;
      break;

      case MUIV_NList_Move_Bottom:
        ent2 = data->NList_Entries - 1;
      break;

      case MUIV_NList_Move_Active:
        ent2 = data->NList_Active;
      break;

      case MUIV_NList_Move_Next:
        ent2 = ent1 + 1;
      break;

      case MUIV_NList_Move_Previous:
        ent2 = ent1 - 1;
      break;

      default:
        ent2 = to;
      break;
    }

    if(ent2 >= 0 && ent2 < data->NList_Entries && isFlagSet(data->EntriesArray[ent2]->Wrap, TE_Wrap_TmpLine))
      ent2 -= data->EntriesArray[ent2]->dnum;

    if(ent2 >= 0 && ent2 < data->NList_Entries && ent1 != ent2)
    {
      struct TypeEntry *newentry;

      data->display_ptr = NULL;

      if(ent1 < ent2)
      {
        if(data->EntriesArray[ent2]->Wrap != 0 && isFlagClear(data->EntriesArray[ent2]->Wrap, TE_Wrap_TmpLine))
          ent2 += (data->EntriesArray[ent2]->dnum - 1);

        newentry = data->EntriesArray[ent1];

        if(data->EntriesArray[ent1]->Wrap != 0 && isFlagClear(data->EntriesArray[ent1]->Wrap, TE_Wrap_TmpLine) && data->EntriesArray[ent1]->len >= 0)
        {
          newentry->pos = (WORD)NL_GetSelects(data, ent1);
          newentry->len = -1;
          data->do_wwrap = TRUE;
        }

        NL_Move(&data->EntriesArray[ent1], &data->EntriesArray[ent1+1], ent2-ent1, ent1);

        data->EntriesArray[ent2] = newentry;
        data->EntriesArray[ent2]->entpos = ent2;

        if(data->NList_Active > ent1 && data->NList_Active <= ent2)
        {
          set_Active(data->NList_Active - 1);
        }
        else if(data->NList_Active == ent1)
        {
          set_Active(ent2);
        }
        NL_SegChanged(data, ent1, ent2);
        data->NList_LastInserted = ent2;
        DO_NOTIFY(NTF_Insert);
      }
      else
      {
        newentry = data->EntriesArray[ent1];

        if(data->EntriesArray[ent1]->Wrap != 0 && isFlagClear(data->EntriesArray[ent1]->Wrap, TE_Wrap_TmpLine) && data->EntriesArray[ent1]->len >= 0)
        {
          newentry->pos = (WORD)NL_GetSelects(data, ent1);
          newentry->len = -1;
          data->do_wwrap = TRUE;
        }

        NL_MoveD(&data->EntriesArray[ent1+1], &data->EntriesArray[ent1], ent1-ent2, ent1+1);

        data->EntriesArray[ent2] = newentry;
        data->EntriesArray[ent2]->entpos = ent2;

        if(data->NList_Active >= ent2 && data->NList_Active < ent1)
        {
          set_Active(data->NList_Active + 1);
        }
        else if(data->NList_Active == ent1)
        {
          set_Active(ent2);
        }
        NL_SegChanged(data,ent2,ent1);
        data->NList_LastInserted = ent2;
        DO_NOTIFY(NTF_Insert);
      }
      data->sorted = FALSE;
      UnSelectCharSel(data, FALSE);
      Make_Active_Visible;
      NL_DoWrapAll(data, FALSE, FALSE);
      data->do_updatesb = TRUE;
      REDRAW;
/*      do_notifies(NTF_AllChanges|NTF_MinMax);*/
      return (TRUE);
    }
  }
  return (FALSE);
}



IPTR mNL_List_Sort(struct IClass *cl, Object *obj, UNUSED struct MUIP_NList_Sort *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Sort(data));
}


IPTR mNL_List_Sort2(struct IClass *cl,Object *obj,struct  MUIP_NList_Sort2 *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  if ((msg->sort_type_add) && ((data->NList_SortType & ~MUIV_NList_SortTypeAdd_Mask) == (ULONG)msg->sort_type))
    data->NList_SortType += msg->sort_type_add;
  else
    data->NList_SortType = msg->sort_type;
  set(obj,MUIA_NList_SortType,data->NList_SortType);
  return (NL_List_Sort(data));
}


IPTR mNL_List_Sort3(struct IClass *cl,Object *obj,struct  MUIP_NList_Sort3 *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  if (msg->which == MUIV_NList_Sort3_SortType_2)
  {
    if ((msg->sort_type_add) && ((data->NList_SortType2 & ~MUIV_NList_SortTypeAdd_Mask) == (ULONG)msg->sort_type))
      data->NList_SortType2 += msg->sort_type_add;
    else
      data->NList_SortType2 = msg->sort_type;
    set(obj,MUIA_NList_SortType2,data->NList_SortType2);
  }
  else
  {
    if ((msg->sort_type_add) && ((data->NList_SortType & ~MUIV_NList_SortTypeAdd_Mask) == (ULONG)msg->sort_type))
      data->NList_SortType += msg->sort_type_add;
    else
      data->NList_SortType = msg->sort_type;
    if (msg->which == MUIV_NList_Sort3_SortType_1)
    { set(obj,MUIA_NList_SortType,data->NList_SortType);
    }
    else
    { data->NList_SortType2 = data->NList_SortType;
      set(obj,MUIA_NList_SortType,data->NList_SortType);
      set(obj,MUIA_NList_SortType2,data->NList_SortType2);
    }
  }
  return (NL_List_Sort(data));
}


IPTR mNL_List_Insert(struct IClass *cl,Object *obj,struct  MUIP_NList_Insert *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Insert(data,msg->entries,msg->count,msg->pos,NOWRAP,ALIGN_LEFT,msg->flags));
}


IPTR mNL_List_InsertSingle(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertSingle *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  if (msg->entry)
    return (NL_List_Insert(data,&(msg->entry),1,msg->pos,NOWRAP,ALIGN_LEFT,0));
  return (0);
}


IPTR mNL_List_InsertWrap(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertWrap *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Insert(data,msg->entries,msg->count,msg->pos,msg->wrapcol,msg->align & ALIGN_MASK,msg->flags));
}


IPTR mNL_List_InsertSingleWrap(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertSingleWrap *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  if (msg->entry)
    return (NL_List_Insert(data,&(msg->entry),1,msg->pos,msg->wrapcol,msg->align & ALIGN_MASK,0));
  return (0);
}


IPTR mNL_List_ReplaceSingle(struct IClass *cl,Object *obj,struct  MUIP_NList_ReplaceSingle *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Replace(data,msg->entry,msg->pos,msg->wrapcol,msg->align & ALIGN_MASK));
}


IPTR mNL_List_Exchange(struct IClass *cl,Object *obj,struct MUIP_NList_Exchange *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Exchange(data,msg->pos1,msg->pos2));
}


IPTR mNL_List_Move(struct IClass *cl,Object *obj,struct MUIP_NList_Move *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Move(data,msg->from,msg->to));
}


IPTR mNL_List_Clear(struct IClass *cl, Object *obj, UNUSED struct MUIP_NList_Clear *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Clear(data));
}


IPTR mNL_List_Remove(struct IClass *cl,Object *obj,struct MUIP_NList_Remove *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_List_Remove(data,msg->pos));
}



#define DROPMARK_NONE    -1
#define DROPMARK_START   -2



IPTR mNL_DragQuery(struct IClass *cl,Object *obj,struct MUIP_DragQuery *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  if (data->NList_Disabled)
  {
/*D(bug("%lx| 1 DragQuery_Refuse\n",obj));*/
    return (MUIV_DragQuery_Refuse);
  }
  if ((msg->obj==obj) && (data->NList_DragSortable))
  {
/*D(bug("%lx| 3 DragQuery_Accept\n",obj));*/
    return(MUIV_DragQuery_Accept);
  }
/*D(bug("%lx| 3 DragQuery_Refuse\n",obj));*/
  return(MUIV_DragQuery_Refuse);
}


IPTR mNL_DragBegin(struct IClass *cl,Object *obj,struct MUIP_DragBegin *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  data->NList_DropMark = DROPMARK_START;
  if (data->NList_Disabled)
    return (0);
  if ((data->NList_ShowDropMarks || (msg->obj==obj)) && (data->NList_Entries > 0))
    return(0);
  return(DoSuperMethodA(cl,obj,(Msg) msg));
}


IPTR mNL_DragReport(struct IClass *cl,Object *obj,struct MUIP_DragReport *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  LONG mdy,type,lyl = DROPMARK_NONE;

  if (data->NList_Disabled)
  {
/*D(bug("%lx| 1 DragReport_Abort\n",obj));*/
    return (MUIV_DragReport_Abort);
  }
  data->dropping = TRUE;
  if (msg->update)
  { LONG drawnum = -1,erasenum = -1;
    if (data->markdraw && (data->markdrawnum >= 0))
      drawnum = data->markdrawnum;
    if (data->markerase && (data->markerasenum >= 0) && (data->markerasenum != drawnum))
      erasenum = data->markerasenum;
    if ((drawnum >= 0) || (erasenum >= 0))
    { if ((DoMethod(obj,MUIM_NList_DropEntryDrawErase, data->marktype,drawnum, erasenum) != MUIM_NList_DropEntryDrawErase) &&
          (drawnum >= 0))
        NL_Changed(data,drawnum);
    }
    data->display_ptr = NULL;
    data->parse_column = -1;
    REDRAW_FORCE;
    if (data->NList_SerMouseFix)
      return(MUIV_DragReport_Continue);
    else
      return(MUIV_DragReport_Lock);
  }
  else if ((msg->x >= _left(obj)) && (msg->x <= _right(obj)))
  { type = MUIV_NList_DropType_Above;
    if (data->NList_DropMark == DROPMARK_START)
    { data->NList_DropMark = DROPMARK_NONE;
      if (data->markdrawnum == (LONG)MUIM_NList_Trigger)
        data->markdrawnum = 0;
      else if ((msg->x > data->mleft+6) && (msg->x < data->mright-6))
      { if (msg->y < data->vpos+4)
          lyl = 0;
        else if (msg->y > data->mbottom-4)
          lyl = data->NList_Entries;
      }
    }
    if (lyl < 0)
    { LONG ly = (msg->y - data->vpos);
      /*lyl = (ly + data->vinc/2) / data->vinc + data->NList_First;*/
      lyl = ly / data->vinc + data->NList_First;
      if ((ly % data->vinc) >= (data->vinc / 2))
        type = MUIV_NList_DropType_Below;
      if (lyl >= data->NList_First + data->NList_Visible)
      { if (msg->y > data->mbottom+40)
          lyl = data->NList_First + data->NList_Visible + 7;
        else if (msg->y > data->mbottom+24)
          lyl = data->NList_First + data->NList_Visible + 3;
        else if (msg->y > data->mbottom+8)
          lyl = data->NList_First + data->NList_Visible + 1;
        else
          lyl = data->NList_First + data->NList_Visible;
      }
      if ((ly < 0) || (lyl < data->NList_First))
      { if (msg->y < data->vpos-40)
          lyl = data->NList_First - 8;
        else if (msg->y < data->vpos-24)
          lyl = data->NList_First - 4;
        else if (msg->y < data->vpos-8)
          lyl = data->NList_First - 2;
        else
          lyl = data->NList_First - 1;
        if (lyl < 0)
          lyl = 0;
      }
    }
    if (data->NList_Entries == 0)
    { lyl = 0;
      type = MUIV_NList_DropType_Above;
    }
    else if (lyl >= data->NList_Entries)
    { lyl = data->NList_Entries - 1;
      type = MUIV_NList_DropType_Below;
    }
    mdy = data->vpos + (data->vinc * (lyl - data->NList_First));
    DoMethod(obj,MUIM_NList_DropType, &lyl,&type, data->mleft,data->mright,
                                      mdy,mdy+data->vinc-1, msg->x,msg->y);
    if ((type & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Below)
    { lyl++;
      type = (type & ~MUIV_NList_DropType_Mask) | MUIV_NList_DropType_Above;
    }
    if (data->NList_Entries == 0)
    { lyl = 0;
      type = (type & ~MUIV_NList_DropType_Mask) | MUIV_NList_DropType_Above;
    }
    else if (lyl >= data->NList_Entries)
    { lyl = data->NList_Entries - 1;
      type = (type & ~MUIV_NList_DropType_Mask) | MUIV_NList_DropType_Below;
    }

    if ((data->NList_DropMark != lyl) || (type != data->marktype))
    { if (data->NList_DropMark >= 0)
        data->markerase = TRUE;
      data->markdraw = TRUE;
      if (lyl >= -1)
        data->NList_DropMark = lyl;
      if (data->NList_DropMark >= 0)
      {
        if (data->NList_DropMark < data->NList_First)
        { data->NList_First = data->NList_DropMark;
          DO_NOTIFY(NTF_First);
        }
        if (data->NList_DropMark >= data->NList_First + data->NList_Visible)
        { data->NList_First = data->NList_DropMark - data->NList_Visible + 1;
          DO_NOTIFY(NTF_First);
        }
        if (data->NList_First < 0)
        { data->NList_First = 0;
          DO_NOTIFY(NTF_First);
        }
        data->markdrawnum = data->NList_DropMark;
        data->marktype = type;
        if ((data->NList_ShowDropMarks || (msg->obj==obj)) && (data->NList_Entries > 0))
          return(MUIV_DragReport_Refresh);
        else
        { data->markerase = FALSE;
          data->markdraw = FALSE;
          if (data->NList_First != data->NList_AffFirst)
            return(MUIV_DragReport_Refresh);
        }
      }
      else if (data->markerase)
      { data->markdraw = FALSE;
        if ((data->NList_ShowDropMarks || (msg->obj==obj)) && (data->NList_Entries > 0))
          return(MUIV_DragReport_Refresh);
      }
    }
    if (data->NList_SerMouseFix)
      return(MUIV_DragReport_Continue);
    else
      return(MUIV_DragReport_Lock);
  }
/*D(bug("%lx| 2 DragReport_Abort\n",obj));*/
  return(MUIV_DragReport_Abort);
}


IPTR mNL_DragFinish(struct IClass *cl,Object *obj,struct MUIP_DragFinish *msg)
{
  register struct NLData *data = INST_DATA(cl,obj);
  if (data->NList_DropMark == DROPMARK_START)
    data->NList_DropMark = DROPMARK_NONE;
  if (data->NList_Disabled)
    return (0);
  if ((data->NList_ShowDropMarks || (msg->obj==obj)) && (data->NList_Entries > 0))
  { if (data->NList_DropMark >= 0)
    { data->markdraw = FALSE;
      data->markerase = TRUE;
      if (data->markerase && (data->markerasenum >= 0))
        DoMethod(obj,MUIM_NList_DropEntryDrawErase, 0,-1,data->markerasenum);
      data->display_ptr = NULL;
      data->parse_column = -1;
      REDRAW_FORCE;
    }
    data->dropping = FALSE;
    return(0);
  }
  data->dropping = FALSE;
  return(DoSuperMethodA(cl,obj,(Msg) msg));
}


IPTR mNL_DragDrop(struct IClass *cl,Object *obj,struct MUIP_DragDrop *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  LONG ent = data->NList_DropMark;

  if (data->NList_Disabled)
    return (0);

  if (data->NList_DragSortable && (msg->obj==obj) && (ent >= 0) && (data->marktype & MUIV_NList_DropType_Mask))
  {
    LONG li,res;
    if ((data->marktype & MUIV_NList_DropType_Mask) == MUIV_NList_DropType_Below)
      ent++;
    li = data->NList_LastInserted;
    data->NList_LastInserted = -1;
    res = NL_List_Move_Selected(data,ent);
    if (data->NList_LastInserted >= 0)
    {
      DO_NOTIFY(NTF_DragSortInsert);
    }
    else
      data->NList_LastInserted = li;

    return ((ULONG)res);
  }
  return(0);
}


IPTR mNL_DropType(UNUSED struct IClass *cl, UNUSED Object *obj, UNUSED struct MUIP_NList_DropType *msg)
{
  /*register struct NLData *data = INST_DATA(cl,obj);*/
  return(0);
}

IPTR mNL_DropEntryDrawErase(UNUSED struct IClass *cl, UNUSED Object *obj, UNUSED struct MUIP_NList_DropEntryDrawErase *msg)
{
  /*register struct NLData *data = INST_DATA(cl,obj);*/
  return(MUIM_NList_DropEntryDrawErase);
}
