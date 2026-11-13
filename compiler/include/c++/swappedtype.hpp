#ifndef AROS_CXX_SWAPPEDTYPE_HPP
#define AROS_CXX_SWAPPEDTYPE_HPP

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: c++ template classes used to manage generalized byteswapping
    Lang: english
*/

/* This is a duplicate of the definition present in exec/types.hpp. 
   The reason for this is explained in said header file.  */
namespace aros
{
    template <typename T>
    struct TypeWrapper;

    template <typename T>
    class SwappedType : private TypeWrapper<T>
    {
        typedef SwappedType<T> ThisType;
        T data;
    
    public:

        enum { isSigned = TypeWrapper<T>::isSigned };
    
        /* The default constructor does nothing */
        SwappedType() {}
    
        /* Constructs a SwappedType from a "raw" type. T2 can be anything
           for which a TypeWrapper class has been writtem.  */
        template <typename T2>
        SwappedType(const T2 &v) : data(bswap(v)) {}

        SwappedType(const ThisType &v) : data(operator =(v).get()) {}
    
        /* Converts the swapped data back to the normal native representation. */
        operator T() const 
        {
            return bswap(data);
        }
        
        /* Assignment operator which works on SwappedType's with the same
           template argument.  */
        ThisType &operator =(const ThisType &v)
        {
            data = v.data;
            return *this;
        }    
    
        /* Assignment operator which takes a "raw" type as argument.  */
        template <typename T2>
        ThisType &operator =(const T2 v)
        {         
            data = bswap(v);
            return *this;
        }    
    
        /* Getter method which gives access to the byteswapped data.  */
        const T &get() const
        {
            return data;
        }
    };
    
    /* Specialization of the SwappedType class which works on 
       pointers.  */
    template <typename T>
    class SwappedType<T *> : private TypeWrapper<T *>
    {
        typedef SwappedType<T *> ThisType;
        T *data;    
    
    public:
        
        enum { isSigned = TypeWrapper<T *>::isSigned };
        
        /* The default constructor does nothing */
        SwappedType() {}
        
        /* Constructs a SwappedType from a "raw" type. T2 can be anything
           for which a TypeWrapper class has been writtem.
           
           The construction will have success as long as T2* is compatible with
           T*.  */
        template <typename T2>
        SwappedType(const T2 *v) : data(bswap(v)) {}
    
        /* Constructs a SwappedType from a void* pointer. This makes
           it possible to assign a NULL pointer to a variable of this class,
           and also all other void* pointers, much like with normal pointers.  */
        SwappedType(const void *v) : data(bswap((T*)v)) {}
    
        /* Constructs a SwappedType from another SwappedType, even when the 
           template argument is different. This will succeed as long as T2* is
           compatible with T*.  */
        template <typename T2>
        SwappedType(const SwappedType<T2 *> &v) : data(v.get()) {}
    
        /* Converts the swapped data back to the normal native representation. */
        operator T *() const throw() 
        {
            return bswap(data);
        }
        
        /* The deference operator. It makes this class really behave as a pointer.  
           I'm using a template here so that the compiler won't instantiate
           the method until it's used. This lets me avoid specialize this 
           whole class just for the void * type (which can't be referenced).  */
        template <typename T2>
        T2 &operator *() const throw()
        {
              return *bswap(data);
        }
    
        /* The indirection operator. Essential for a pointer-like class.  */
        T *operator ->() const throw()
        {
            return bswap(data);
        }
                        
        /* Assignment operator which works on SwappedType's even with different
           template arguments. This will succeed as long as T2* is compatible with
           T*. */
        template <typename T2> 
        ThisType &operator =(const SwappedType<T2 *> &v)
        {
            data = v.get();
            return *this;
        }    
    
        /* Assignment operator which takes a "raw" type as argument.  
           This will succeed as long as T2* is compatible with T*. */
        template <typename T2>
        ThisType &operator =(const T2 *v)
        {         
            data = bswap(v);
            return *this;
        }    
        
        /* Getter method which gives access to the byteswapped data.  */
        T * get() const
        {
            return data;
        }
    };
}
#endif /* !AROS_CXX_SWAPPEDTYPE_HPP */
