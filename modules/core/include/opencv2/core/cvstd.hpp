/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef OPENCV_CORE_CVSTD_HPP
#define OPENCV_CORE_CVSTD_HPP

#ifndef __cplusplus
#  error cvstd.hpp header must be compiled as C++
#endif

#include "opencv2/core/cvdef.h"
#include <cstddef>
#include <cstring>
#include <cctype>

#include <string>

// import useful primitives from stl
#  include <algorithm>
#  include <utility>
#  include <cstdlib> //for abs(int)
#  include <cmath>

namespace cv
{
    static inline uchar abs(uchar a) { return a; }
    static inline ushort abs(ushort a) { return a; }
    static inline unsigned abs(unsigned a) { return a; }
    static inline uint64 abs(uint64 a) { return a; }

    using std::min;
    using std::max;
    using std::abs;
    using std::swap;
    using std::sqrt;
    using std::exp;
    using std::pow;
    using std::log;
}

namespace cv {

//! @addtogroup core_utils
//! @{

//////////////////////////// memory management functions ////////////////////////////

/** @brief Allocates an aligned memory buffer.

The function allocates the buffer of the specified size and returns it. When the buffer size is 16
bytes or more, the returned buffer is aligned to 16 bytes.
@param bufSize Allocated buffer size.
 */
CV_EXPORTS void* fastMalloc(size_t bufSize);

/** @brief Deallocates a memory buffer.

The function deallocates the buffer allocated with fastMalloc . If NULL pointer is passed, the
function does nothing. C version of the function clears the pointer *pptr* to avoid problems with
double memory deallocation.
@param ptr Pointer to the allocated buffer.
 */
CV_EXPORTS void fastFree(void* ptr);

/*!
  The STL-compilant memory Allocator based on cv::fastMalloc() and cv::fastFree()
*/
template<typename _Tp> class Allocator
{
public:
    typedef _Tp value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    template<typename U> class rebind { typedef Allocator<U> other; };

    explicit Allocator() {}
    ~Allocator() {}
    explicit Allocator(Allocator const&) {}
    template<typename U>
    explicit Allocator(Allocator<U> const&) {}

    // address
    pointer address(reference r) { return &r; }
    const_pointer address(const_reference r) { return &r; }

    pointer allocate(size_type count, const void* =0) { return reinterpret_cast<pointer>(fastMalloc(count * sizeof (_Tp))); }
    void deallocate(pointer p, size_type) { fastFree(p); }

    void construct(pointer p, const _Tp& v) { new(static_cast<void*>(p)) _Tp(v); }
    void destroy(pointer p) { p->~_Tp(); }

    size_type max_size() const { return cv::max(static_cast<_Tp>(-1)/sizeof(_Tp), 1); }
};

//! @} core_utils

//! @cond IGNORED

namespace detail
{

// Metafunction to avoid taking a reference to void.
template<typename T>
struct RefOrVoid { typedef T& type; };

template<>
struct RefOrVoid<void>{ typedef void type; };

template<>
struct RefOrVoid<const void>{ typedef const void type; };

template<>
struct RefOrVoid<volatile void>{ typedef volatile void type; };

template<>
struct RefOrVoid<const volatile void>{ typedef const volatile void type; };

// This class would be private to Ptr, if it didn't have to be a non-template.
struct PtrOwner;

}

template<typename Y>
struct DefaultDeleter
{
    void operator () (Y* p) const;
};

//! @endcond

//! @addtogroup core_basic
//! @{

/** @brief Template class for smart pointers with shared ownership

A Ptr\<T\> pretends to be a pointer to an object of type T. Unlike an ordinary pointer, however, the
object will be automatically cleaned up once all Ptr instances pointing to it are destroyed.

Ptr is similar to boost::shared_ptr that is part of the Boost library
(<http://www.boost.org/doc/libs/release/libs/smart_ptr/shared_ptr.htm>) and std::shared_ptr from
the [C++11](http://en.wikipedia.org/wiki/C++11) standard.

This class provides the following advantages:
-   Default constructor, copy constructor, and assignment operator for an arbitrary C++ class or C
    structure. For some objects, like files, windows, mutexes, sockets, and others, a copy
    constructor or an assignment operator are difficult to define. For some other objects, like
    complex classifiers in OpenCV, copy constructors are absent and not easy to implement. Finally,
    some of complex OpenCV and your own data structures may be written in C. However, copy
    constructors and default constructors can simplify programming a lot. Besides, they are often
    required (for example, by STL containers). By using a Ptr to such an object instead of the
    object itself, you automatically get all of the necessary constructors and the assignment
    operator.
-   *O(1)* complexity of the above-mentioned operations. While some structures, like std::vector,
    provide a copy constructor and an assignment operator, the operations may take a considerable
    amount of time if the data structures are large. But if the structures are put into a Ptr, the
    overhead is small and independent of the data size.
-   Automatic and customizable cleanup, even for C structures. See the example below with FILE\*.
-   Heterogeneous collections of objects. The standard STL and most other C++ and OpenCV containers
    can store only objects of the same type and the same size. The classical solution to store
    objects of different types in the same container is to store pointers to the base class (Base\*)
    instead but then you lose the automatic memory management. Again, by using Ptr\<Base\> instead
    of raw pointers, you can solve the problem.

A Ptr is said to *own* a pointer - that is, for each Ptr there is a pointer that will be deleted
once all Ptr instances that own it are destroyed. The owned pointer may be null, in which case
nothing is deleted. Each Ptr also *stores* a pointer. The stored pointer is the pointer the Ptr
pretends to be; that is, the one you get when you use Ptr::get or the conversion to T\*. It's
usually the same as the owned pointer, but if you use casts or the general shared-ownership
constructor, the two may diverge: the Ptr will still own the original pointer, but will itself point
to something else.

The owned pointer is treated as a black box. The only thing Ptr needs to know about it is how to
delete it. This knowledge is encapsulated in the *deleter* - an auxiliary object that is associated
with the owned pointer and shared between all Ptr instances that own it. The default deleter is an
instance of DefaultDeleter, which uses the standard C++ delete operator; as such it will work with
any pointer allocated with the standard new operator.

However, if the pointer must be deleted in a different way, you must specify a custom deleter upon
Ptr construction. A deleter is simply a callable object that accepts the pointer as its sole
argument. For example, if you want to wrap FILE, you may do so as follows:
@code
    Ptr<FILE> f(fopen("myfile.txt", "w"), fclose);
    if(!f) throw ...;
    fprintf(f, ....);
    ...
    // the file will be closed automatically by f's destructor.
@endcode
Alternatively, if you want all pointers of a particular type to be deleted the same way, you can
specialize DefaultDeleter<T>::operator() for that type, like this:
@code
    namespace cv {
    template<> void DefaultDeleter<FILE>::operator ()(FILE * obj) const
    {
        fclose(obj);
    }
    }
@endcode
For convenience, the following types from the OpenCV C API already have such a specialization that
calls the appropriate release function:
-   CvCapture
-   CvFileStorage
-   CvHaarClassifierCascade
-   CvMat
-   CvMatND
-   CvMemStorage
-   CvSparseMat
-   CvVideoWriter
-   IplImage
@note The shared ownership mechanism is implemented with reference counting. As such, cyclic
ownership (e.g. when object a contains a Ptr to object b, which contains a Ptr to object a) will
lead to all involved objects never being cleaned up. Avoid such situations.
@note It is safe to concurrently read (but not write) a Ptr instance from multiple threads and
therefore it is normally safe to use it in multi-threaded applications. The same is true for Mat and
other C++ OpenCV classes that use internal reference counts.
*/
template<typename T>
struct Ptr
{
    /** Generic programming support. */
    typedef T element_type;

    /** The default constructor creates a null Ptr - one that owns and stores a null pointer.
    */
    Ptr();

    /**
    If p is null, these are equivalent to the default constructor.
    Otherwise, these constructors assume ownership of p - that is, the created Ptr owns and stores p
    and assumes it is the sole owner of it. Don't use them if p is already owned by another Ptr, or
    else p will get deleted twice.
    With the first constructor, DefaultDeleter\<Y\>() becomes the associated deleter (so p will
    eventually be deleted with the standard delete operator). Y must be a complete type at the point
    of invocation.
    With the second constructor, d becomes the associated deleter.
    Y\* must be convertible to T\*.
    @param p Pointer to own.
    @note It is often easier to use makePtr instead.
     */
    template<typename Y>
#ifdef DISABLE_OPENCV_24_COMPATIBILITY
    explicit
#endif
    Ptr(Y* p);

    /** @overload
    @param d Deleter to use for the owned pointer.
    @param p Pointer to own.
    */
    template<typename Y, typename D>
    Ptr(Y* p, D d);

    /**
    These constructors create a Ptr that shares ownership with another Ptr - that is, own the same
    pointer as o.
    With the first two, the same pointer is stored, as well; for the second, Y\* must be convertible
    to T\*.
    With the third, p is stored, and Y may be any type. This constructor allows to have completely
    unrelated owned and stored pointers, and should be used with care to avoid confusion. A relatively
    benign use is to create a non-owning Ptr, like this:
    @code
        ptr = Ptr<T>(Ptr<T>(), dont_delete_me); // owns nothing; will not delete the pointer.
    @endcode
    @param o Ptr to share ownership with.
    */
    Ptr(const Ptr& o);

    /** @overload
    @param o Ptr to share ownership with.
    */
    template<typename Y>
    Ptr(const Ptr<Y>& o);

    /** @overload
    @param o Ptr to share ownership with.
    @param p Pointer to store.
    */
    template<typename Y>
    Ptr(const Ptr<Y>& o, T* p);

    /** The destructor is equivalent to calling Ptr::release. */
    ~Ptr();

    /**
    Assignment replaces the current Ptr instance with one that owns and stores same pointers as o and
    then destroys the old instance.
    @param o Ptr to share ownership with.
     */
    Ptr& operator = (const Ptr& o);

    /** @overload */
    template<typename Y>
    Ptr& operator = (const Ptr<Y>& o);

    /** If no other Ptr instance owns the owned pointer, deletes it with the associated deleter. Then sets
    both the owned and the stored pointers to NULL.
    */
    void release();

    /**
    `ptr.reset(...)` is equivalent to `ptr = Ptr<T>(...)`.
    @param p Pointer to own.
    */
    template<typename Y>
    void reset(Y* p);

    /** @overload
    @param d Deleter to use for the owned pointer.
    @param p Pointer to own.
    */
    template<typename Y, typename D>
    void reset(Y* p, D d);

    /**
    Swaps the owned and stored pointers (and deleters, if any) of this and o.
    @param o Ptr to swap with.
    */
    void swap(Ptr& o);

    /** Returns the stored pointer. */
    T* get() const;

    /** Ordinary pointer emulation. */
    typename detail::RefOrVoid<T>::type operator * () const;

    /** Ordinary pointer emulation. */
    T* operator -> () const;

    /** Equivalent to get(). */
    operator T* () const;

    /** ptr.empty() is equivalent to `!ptr.get()`. */
    bool empty() const;

    /** Returns a Ptr that owns the same pointer as this, and stores the same
       pointer as this, except converted via static_cast to Y*.
    */
    template<typename Y>
    Ptr<Y> staticCast() const;

    /** Ditto for const_cast. */
    template<typename Y>
    Ptr<Y> constCast() const;

    /** Ditto for dynamic_cast. */
    template<typename Y>
    Ptr<Y> dynamicCast() const;

#ifdef CV_CXX_MOVE_SEMANTICS
    Ptr(Ptr&& o);
    Ptr& operator = (Ptr&& o);
#endif

private:
    detail::PtrOwner* owner;
    T* stored;

    template<typename Y>
    friend struct Ptr; // have to do this for the cross-type copy constructor
};

/** Equivalent to ptr1.swap(ptr2). Provided to help write generic algorithms. */
template<typename T>
void swap(Ptr<T>& ptr1, Ptr<T>& ptr2);

/** Return whether ptr1.get() and ptr2.get() are equal and not equal, respectively. */
template<typename T>
bool operator == (const Ptr<T>& ptr1, const Ptr<T>& ptr2);
template<typename T>
bool operator != (const Ptr<T>& ptr1, const Ptr<T>& ptr2);

/** `makePtr<T>(...)` is equivalent to `Ptr<T>(new T(...))`. It is shorter than the latter, and it's
marginally safer than using a constructor or Ptr::reset, since it ensures that the owned pointer
is new and thus not owned by any other Ptr instance.
Unfortunately, perfect forwarding is impossible to implement in C++03, and so makePtr is limited
to constructors of T that have up to 10 arguments, none of which are non-const references.
 */
template<typename T>
Ptr<T> makePtr();
/** @overload */
template<typename T, typename A1>
Ptr<T> makePtr(const A1& a1);
/** @overload */
template<typename T, typename A1, typename A2>
Ptr<T> makePtr(const A1& a1, const A2& a2);
/** @overload */
template<typename T, typename A1, typename A2, typename A3>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9);
/** @overload */
template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9, const A10& a10);

//////////////////////////////// string class ////////////////////////////////

class CV_EXPORTS FileNode; //for string constructor from FileNode

typedef std::string String;

//! @} core_basic
} // cv

#include "opencv2/core/ptr.inl.hpp"

#endif //OPENCV_CORE_CVSTD_HPP
