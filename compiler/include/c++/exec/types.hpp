#ifndef AROS_CXX_EXEC_TYPES_HPP
#define AROS_CXX_EXEC_TYPES_HPP

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: big and little endian type definitions for use by programs
          that need to access foreign-endianess data. 
    Lang: english
*/

#include <exec/types.h>
#include <aros/macros.h>

/* This is a duplicate of the definition present in swappedtypes.hpp. 
   The reason for this is explained further below.  */
namespace aros
{
    template <typename T>
    struct TypeWrapper;

    /* Specialized versions of the TypeWrapper functor.  */
    template <>
    struct TypeWrapper<UWORD>
    {
        enum { isSigned = false };
    
        UWORD bswap(const UWORD v) const
        {
            return AROS_SWAP_BYTES_WORD(v);
        }
        
        UWORD ones() const
        {
            return (UWORD)-1;
        }    
    };   
    
    template <>
    struct TypeWrapper<WORD>
    {
        enum { isSigned = true };
    
        WORD bswap(const WORD v) const
        {
            return AROS_SWAP_BYTES_WORD(v);
        }
        
        UWORD ones() const
        {
            return (UWORD)-1;
        }    
    };   
    
    template<>
    struct TypeWrapper<ULONG>
    {
        enum { isSigned = false };
        
        ULONG bswap(const ULONG v) const
        {
            return AROS_SWAP_BYTES_LONG(v);
        }
        
        ULONG ones() const
        {
            return (ULONG)-1;
        }    
    };
    
    template <>
    struct TypeWrapper<LONG>
    {
        enum { isSigned = true };
    
        LONG bswap(const LONG v) const
        {
           return AROS_SWAP_BYTES_LONG(v);
        }
        
        ULONG ones() const
        {
            return (ULONG)-1;
        }    
    };
        
    template<>
    struct TypeWrapper<UQUAD>
    {
        enum { isSigned = false };
        
        const UQUAD bswap(const UQUAD v) const
        {
            return AROS_SWAP_BYTES_QUAD(v);
        }
        
        const UQUAD ones() const
        {
            return (UQUAD)-1;
        }    
    };
    
    template<>
    struct TypeWrapper<QUAD>
    {
        enum { isSigned = true };
        
        const QUAD bswap(const QUAD v) const
        {
            return AROS_SWAP_BYTES_QUAD(v);
        }
        
        const UQUAD ones() const
        {
            return (UQUAD)-1;
        }    
    };
    
    template <typename T, unsigned int size>
    struct TypeWrapperPtr;
    
    template <typename T>
    struct TypeWrapperPtr<T, sizeof(ULONG)>
    {
        T *bswap(const T *v) const
        {
            return (T *)AROS_SWAP_BYTES_LONG((ULONG)v);
        }                
    };    
    
    template <typename T>
    struct TypeWrapperPtr<T, sizeof(UQUAD)>
    {
        T *bswap(const T *v) const
        {
            return (T *)AROS_SWAP_BYTES_QUAD((UQUAD)v);
        }                
    };    
    
    template <typename T>
    struct TypeWrapper<T *> : TypeWrapperPtr<T, sizeof(T *)> 
    {
        enum { isSigned = false };
    
        T *ones() const
        {
            return (T *)(IPTR)-1;        
        }        
    };
}    
/* For some odd reasons gcc will complain if I put this at the top, because it won't see that
   TypeWrapper<T *> inherits from TypeWrapperPtr<T, sizeof(T *)>.
       
   Is this a bug or a feature?!  */
#include <c++/swappedtype.hpp>
    
    
    /* Some useful types, in their big and little endian version.

   You can use them just like normal types. Of course, though, it makes
   no much sense to declare, say, a BELONG variable, since that will only
   waste cpu cycles for no useful purpose. You'll instead want to declare
   pointers to variables of these types, or even use these types as types
   of the fields of certain structures which are known to be little/big
   endian: doing so, will save you lot of extra typing and many sutle bugs,
   and will also make the code look cleaner.  
   
   Use BEPTR(type) or LEPTR(type) to declare big/little endian pointers
   to data.
   
   WARNING: this is *NOT* the same as pointers to big/little endian data!!
            In this case it's the pointer itself that is in big/little
            endian mode.
   
   You won't really find much use for this particular thing, unless you need
   to fiddle with "live" foreign-endian program's data, which might be the case,
   for example, with emulators.  */

#if AROS_BIG_ENDIAN

typedef aros::SwappedType<WORD>  LEWORD;
typedef aros::SwappedType<LONG>  LELONG;
typedef aros::SwappedType<QUAD>  LEQUAD;

typedef aros::SwappedType<UWORD> LEUWORD;
typedef aros::SwappedType<ULONG> LEULONG;
typedef aros::SwappedType<UQUAD> LEUQUAD;

typedef aros::SwappedType<APTR>  LEAPTR;

typedef WORD  BEWORD;               
typedef LONG  BELONG;
typedef QUAD  BEQUAD;  

typedef UWORD BEUWORD;               
typedef ULONG BEULONG;
typedef UQUAD BEUQUAD;

typedef APTR  BEAPTR;

#define BEPTR(type) type *
#define LEPTR(type) aros::SwappedType<type *>

#else

typedef aros::SwappedType<WORD>  BEWORD;
typedef aros::SwappedType<LONG>  BELONG;
typedef aros::SwappedType<QUAD>  BEQUAD;

typedef aros::SwappedType<UWORD> BEUWORD;
typedef aros::SwappedType<ULONG> BEULONG;
typedef aros::SwappedType<UQUAD> BEUQUAD;

typedef aros::SwappedType<APTR>  BEAPTR;

typedef WORD  LEWORD;               
typedef LONG  LELONG;               
typedef QUAD  LEQUAD;               

typedef UWORD LEUWORD;               
typedef ULONG LEULONG;
typedef UQUAD LEUQUAD;

typedef APTR  LEAPTR;

#define BEPTR(type) aros::SwappedType<type *>
#define LEPTR(type) type *

#endif

typedef BYTE  LEBYTE;
typedef BYTE  BEBYTE;
typedef UBYTE LEUBYTE;
typedef UBYTE BEUBYTE;

#endif /* AROS_CXX_EXEC_TYPES_HPP  */
