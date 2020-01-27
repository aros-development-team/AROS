

#include <proto/exec.h>

#include <aros/debug.h>
#include <aros/macros.h>

#include <limits.h>
#include <string.h>

#include "kms_intern.h"

#if !AROS_BIG_ENDIAN || (__WORDSIZE != 32)

#define SKIPLONG(ptr)	ptr += sizeof(LONG);

#define SKIPWORD(ptr)	ptr += sizeof(WORD)

#define SKIPBYTE(ptr)	ptr ++;

#define SKIPPTR(ptr) 	ptr += sizeof(LONG)

#if AROS_BIG_ENDIAN     
// BE conversion functions.
#define CONVLONG(ptr, destlong)  \
destlong = (*((ULONG *)(ptr))); \
SKIPLONG(ptr);

#define CONVWORD(ptr, destword) \
destword = (*((UWORD *)(ptr))); \
SKIPWORD(ptr);

#define CONVBYTE(ptr, destbyte) \
destbyte = ptr[0]; \
SKIPBYTE(ptr);

#define COPYPTR(ptr, destptr) \
(destptr) = (APTR)((IPTR)0 + *((ULONG *)(ptr))); \
SKIPPTR(ptr);
#else
// LE conversion functions.
#define CONVLONG(ptr, destlong)  \
destlong = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3]; \
SKIPLONG(ptr);

#define CONVWORD(ptr, destword) \
destword = ptr[0] <<  8 | ptr[1]; \
SKIPWORD(ptr);

#define CONVBYTE(ptr, destbyte) \
destbyte = ptr[0]; \
SKIPBYTE(ptr);

#define COPYPTR(ptr, destptr) \
(destptr) = (APTR)((IPTR)0 + ptr[0] << 24 | ptr[1] << 16 | ptr [2] << 8 | ptr[3]); \
SKIPPTR(ptr);
#endif

UWORD countstrdescr(UBYTE *descr, UBYTE cnt)
{
   IPTR end = (IPTR)descr, tmp;
   UWORD i, entries;

   D(bug("[KMS] %s: count = %d\n", __func__, cnt);)
   entries = cnt << 1;
   D(bug("[KMS] %s:       = %d entries\n", __func__, entries);)

   for (i = 0; i < entries; i++)
   {
      tmp = (IPTR)descr + descr[(i * 2)] + descr[(i * 2) + 1];
      if (tmp > end)
         end = tmp;
   }
   D(bug("[KMS] %s: %d bytes\n", __func__, (end - (IPTR)descr) + 1);)
   return (end - (IPTR)descr) + 1;
}

UWORD countdkdescr(UBYTE *descr, UBYTE cnt)
{
   IPTR end = (IPTR)descr, tmp;
   UWORD i, entries;

   D(bug("[KMS] %s: count = %d\n", __func__, cnt);)
   entries = 1 << cnt;
   D(bug("[KMS] %s:       = %d entries\n", __func__, entries);)

   for (i = 0; i < entries; i++)
   {
      switch (descr[(i * 2)])
      {
         case 0:
         case DPF_DEAD:
            {
               tmp = (IPTR)&descr[(i * 2) + 1];
               if (tmp > end)
                  end = tmp;
            }
            break;
         case DPF_MOD:
            {
               tmp = (IPTR)descr + descr[(i * 2) + 1] + 18;
               if (tmp > end)
                  end = tmp;
            }
            break;
      }
   }
   D(bug("[KMS] %s: %d bytes\n", __func__, (end - (IPTR)descr) + 1);)
   return (end - (IPTR)descr) + 1;
}

BPTR parsekeymapseg(BPTR km_seg)
{
   struct KeyMapNode tmp_nativekm = { 0 }, *kmnative = NULL;
   BPTR kmnative_seg = BNULL;

   D(bug("[KMS] %s(0x%p)\n", __func__, km_seg);)
   if (km_seg && (IPTR)BADDR(km_seg) < UINT_MAX)
   {
      ULONG kmsegsize = sizeof(BPTR) + sizeof(struct KeyMapNode);
      APTR alloctmp;
      UBYTE *ptr, *kmptr, *tmpptr, keyval;
      int i, size;

      /* Clear temporary keymap struct */
      memset(&tmp_nativekm, 0, sizeof (struct KeyMapNode));

      /* Get start of keymap. */ 
      ptr = (UBYTE *)(BADDR(km_seg)) + sizeof(BPTR);

      SKIPPTR(ptr);	    	    	        /* kn_Node.ln_Succ	*/
      SKIPPTR(ptr);	    	    	        /* kn_Node.ln_Pred	*/
      CONVBYTE(ptr, tmp_nativekm.kn_Node.ln_Type);  /* kn_Node.ln_Type	*/	
      CONVBYTE(ptr, tmp_nativekm.kn_Node.ln_Pri);   /* kn_Node.ln_Pri	*/
      COPYPTR(ptr, tmp_nativekm.kn_Node.ln_Name);   /* kn_Node.ln_Name	*/
      D(bug("[KMS] %s: keymap name @ 0x%p = '%s'\n", __func__, tmp_nativekm.kn_Node.ln_Name, tmp_nativekm.kn_Node.ln_Name);)
      kmsegsize += strlen(tmp_nativekm.kn_Node.ln_Name) + 1;
      if (!strlen(tmp_nativekm.kn_Node.ln_Name) & 0x1)
         kmsegsize += 1;

      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_LoKeyMapTypes);
      D(bug("[KMS] %s: km_LoKeyMapTypes @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_LoKeyMapTypes);)
      kmsegsize += 0x40;
         COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_LoKeyMap);
         D(bug("[KMS] %s: km_LoKeyMap @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_LoKeyMap);)
         kmsegsize += (0x40 * sizeof(IPTR));
         for (i = 0; i < 0x40; i++)
         {
            tmpptr = (UBYTE *)tmp_nativekm.kn_KeyMap.km_LoKeyMapTypes;
            keyval = tmpptr[i];
            D(bug("[KMS] %s: lktype #%02u = %02x \n", __func__, i, keyval);)
            if (keyval & (KCF_STRING|KCF_DEAD))
            {
               IPTR keym;
               ULONG *tmpkeym = (ULONG *)tmp_nativekm.kn_KeyMap.km_LoKeyMap;
               void **lkptr = (void **)&keym;
               kmptr = (UBYTE *)&tmpkeym[i];
               COPYPTR(kmptr, *lkptr);
               D(bug("[KMS] %s: %02u: lk descr @ %p\n", __func__, i, keym);)
               tmpptr = (UBYTE *)keym;
               if (keyval & KCF_STRING)
               {
                  size = countstrdescr(tmpptr, __builtin_popcount(keyval));
                  if (size & 0x1) size += 1;
                  kmsegsize += size;
               }
               else
               {
                  size = countdkdescr(tmpptr, __builtin_popcount(keyval));
                  if (size & 0x1) size += 1;
                  kmsegsize += size;
               }
            }
         }
      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_LoCapsable);
      D(bug("[KMS] %s: km_LoCapsable @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_LoCapsable);)
      kmsegsize += 8;
      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_LoRepeatable);
      D(bug("[KMS] %s: km_LoRepeatable @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_LoRepeatable);)
      kmsegsize += 8;
      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_HiKeyMapTypes);
      D(bug("[KMS] %s: km_HiKeyMapTypes @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_HiKeyMapTypes);)
      kmsegsize += 0x40;
         COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_HiKeyMap);
         D(bug("[KMS] %s: km_HiKeyMap @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_HiKeyMap);)
         kmsegsize += (0x40 * sizeof(IPTR));
         for (i = 0; i < 0x38; i++)
         {
            tmpptr = (UBYTE *)tmp_nativekm.kn_KeyMap.km_HiKeyMapTypes;
            keyval = tmpptr[i];
            D(bug("[KMS] %s: hktype #%02u = %02x \n", __func__, i, keyval);)
            if (keyval & (KCF_STRING|KCF_DEAD))
            {
               IPTR keym;
               ULONG *tmpkeym = (ULONG *)tmp_nativekm.kn_KeyMap.km_HiKeyMap;
               void **hkptr = (void **)&keym;
               kmptr = (UBYTE *)&tmpkeym[i];
               COPYPTR(kmptr, *hkptr);
               D(bug("[KMS] %s: %02u: hk descr @ %p\n", __func__, i, keym);)
               tmpptr = (UBYTE *)keym;
               if (keyval & KCF_STRING)
                  kmsegsize += countstrdescr(tmpptr, __builtin_popcount(keyval));
               else
                  kmsegsize += countdkdescr(tmpptr, __builtin_popcount(keyval));
            }
         }
      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_HiCapsable);
      D(bug("[KMS] %s: km_HiCapsable @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_HiCapsable);)
      kmsegsize += 8;
      COPYPTR(ptr, tmp_nativekm.kn_KeyMap.km_HiRepeatable);
      D(bug("[KMS] %s: km_HiRepeatable @ 0x%p\n", __func__, tmp_nativekm.kn_KeyMap.km_HiRepeatable);)
      kmsegsize += 8;

      /* allocate our new segment/structure and populate it .. */
      if ((alloctmp = AllocMem(kmsegsize, MEMF_CLEAR)) != NULL)
      {
         kmnative = (struct KeyMapNode *)((IPTR)alloctmp + sizeof(BPTR));
         D(bug("[KMS] %s: allocated %d bytes @ 0x%p for native KeyMapNode\n", __func__, kmsegsize, kmnative);)
         ptr = (UBYTE *)&kmnative[1];
         D(bug("[KMS] %s: data starts @ 0x%p\n", __func__, ptr);)
         kmnative->kn_Node.ln_Pri = tmp_nativekm.kn_Node.ln_Pri;
         kmnative->kn_Node.ln_Type = tmp_nativekm.kn_Node.ln_Type;
         kmnative->kn_Node.ln_Name = (char *)((IPTR)alloctmp + kmsegsize - (strlen(tmp_nativekm.kn_Node.ln_Name) + 1));
         if ((IPTR)kmnative->kn_Node.ln_Name & 0x1)
            kmnative->kn_Node.ln_Name -= 1;
         strlcpy(kmnative->kn_Node.ln_Name, tmp_nativekm.kn_Node.ln_Name, strlen(tmp_nativekm.kn_Node.ln_Name) + 1);
         kmnative->kn_KeyMap.km_LoKeyMapTypes = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_LoKeyMapTypes, (APTR)kmnative->kn_KeyMap.km_LoKeyMapTypes, 0x40);
         ptr += 0x40;
         kmnative->kn_KeyMap.km_LoKeyMap = (IPTR *)ptr;
         //CopyMem(tmp_nativekm.kn_KeyMap.km_LoKeyMap, (APTR)kmnative->kn_KeyMap.km_LoKeyMap, 0x40 * sizeof(IPTR));
         ptr += (0x40 * sizeof(IPTR));
         kmnative->kn_KeyMap.km_LoCapsable = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_LoCapsable, (APTR)kmnative->kn_KeyMap.km_LoCapsable, 8);
         ptr += 8;
         kmnative->kn_KeyMap.km_LoRepeatable = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_LoRepeatable, (APTR)kmnative->kn_KeyMap.km_LoRepeatable, 8);
         ptr += 8;
         kmnative->kn_KeyMap.km_HiKeyMapTypes = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_HiKeyMapTypes, (APTR)kmnative->kn_KeyMap.km_HiKeyMapTypes, 0x38);
         ptr += 0x40;
         kmnative->kn_KeyMap.km_HiKeyMap = (IPTR *)ptr;
         //CopyMem(tmp_nativekm.kn_KeyMap.km_HiKeyMap, (APTR)kmnative->kn_KeyMap.km_HiKeyMap, 0x38 * sizeof(IPTR));
         ptr += (0x40 * sizeof(IPTR));
         kmnative->kn_KeyMap.km_HiCapsable = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_HiCapsable, (APTR)kmnative->kn_KeyMap.km_HiCapsable, 7);
         ptr += 8;
         kmnative->kn_KeyMap.km_HiRepeatable = ptr;
         CopyMem(tmp_nativekm.kn_KeyMap.km_HiRepeatable, (APTR)kmnative->kn_KeyMap.km_HiRepeatable, 7);
         ptr += 8;

         /* finally handle the "strings" in km_LoKeyMap & km_HiKeyMap */
         for (i = 0; i < 0x40; i ++)
         {
            ULONG *tmpkeym = (ULONG *)tmp_nativekm.kn_KeyMap.km_LoKeyMap;
            if (kmnative->kn_KeyMap.km_LoKeyMapTypes[i] & (KCF_STRING|KCF_DEAD))
            {
               IPTR keym;
               void **lkptr = (void **)&keym;
               kmptr = (UBYTE *)&tmpkeym[i];
               COPYPTR(kmptr, *lkptr);
               D(bug("[KMS] %s: lk %02u: %p\n", __func__, i, keym);)
               UWORD descrsize;
               tmpptr = (UBYTE *)keym;
               if (kmnative->kn_KeyMap.km_LoKeyMapTypes[i] & KCF_STRING)
                  descrsize = countstrdescr(tmpptr, __builtin_popcount(kmnative->kn_KeyMap.km_LoKeyMapTypes[i]));
               else
                  descrsize = countdkdescr(tmpptr, __builtin_popcount(kmnative->kn_KeyMap.km_LoKeyMapTypes[i]));
               CopyMem((APTR)tmpptr, ptr, descrsize);
               lkptr = (void **)&kmnative->kn_KeyMap.km_LoKeyMap[i];
               *lkptr = ptr;
               ptr += descrsize;
               if (descrsize & 0x1)
                  ptr += 1;
               D(bug("[KMS] %s:        -> %p (%d bytes)\n", __func__, kmnative->kn_KeyMap.km_LoKeyMap[i], descrsize);)
            }
            else
            {
               IPTR *tmplk = (IPTR *)&kmnative->kn_KeyMap.km_LoKeyMap[i];
               *tmplk = (IPTR)AROS_BE2LONG(tmpkeym[i]);
            }
         }
         for (i = 0; i < 0x38; i ++)
         {
            ULONG *tmpkeym = (ULONG *)tmp_nativekm.kn_KeyMap.km_HiKeyMap;
            if (kmnative->kn_KeyMap.km_HiKeyMapTypes[i] & (KCF_STRING|KCF_DEAD))
            {
               IPTR keym;
               void **hkptr = (void **)&keym;
               kmptr = (UBYTE *)&tmpkeym[i];
               COPYPTR(kmptr, *hkptr);
               D(bug("[KMS] %s: hk %02u: %p\n", __func__, i, keym);)
               UWORD descrsize;
               tmpptr = (UBYTE *)keym;
               if (kmnative->kn_KeyMap.km_HiKeyMapTypes[i] & KCF_STRING)
                  descrsize = countstrdescr(tmpptr, __builtin_popcount(kmnative->kn_KeyMap.km_HiKeyMapTypes[i]));
               else
                  descrsize = countdkdescr(tmpptr, __builtin_popcount(kmnative->kn_KeyMap.km_HiKeyMapTypes[i]));
               CopyMem((APTR)tmpptr, ptr, descrsize);
               hkptr = (void **)&kmnative->kn_KeyMap.km_HiKeyMap[i];
               *hkptr = ptr;
               ptr += descrsize;
               if (descrsize & 0x1)
                  ptr += 1;
               D(bug("[KMS] %s:        -> %p (%d bytes)\n", __func__, kmnative->kn_KeyMap.km_HiKeyMap[i], descrsize);)
            }
            else
            {
               IPTR *tmplk = (IPTR *)&kmnative->kn_KeyMap.km_HiKeyMap[i];
               *tmplk = (IPTR)AROS_BE2LONG(tmpkeym[i]);
            }
         }
      }
      kmnative_seg = (BPTR)alloctmp;
   }
   return  kmnative_seg;
};
#endif
