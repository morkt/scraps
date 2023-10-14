// -*- C++ -*-
//! \file       sysmemmap.h
//! \date       Mon Dec 11 22:49:52 2006
//! \brief      memory mapped objects interface.
//
// Copyright (C) 2006 by poddav
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#ifndef SYSMEMMAP_H
#define SYSMEMMAP_H

#include "sysmmdetail.h"
#include "sysio.h"
#include "syserror.h"
#include <stdexcept>
#include <cassert>

namespace sys { namespace mapping {

typedef detail::map_impl::off_type	off_type;
typedef detail::map_impl::size_type	size_type;

inline size_type page_size () { return detail::map_impl::page_size(); }

/// \class sys::mapping::map_base
///
/// base class for memory mapped object.
/// map_base maintains reference counted implementation of mapped object.
/// mapped object is destroyed when there's no views on top of it.

class SYSPP_DLLIMPORT map_base
{
    refcount_ptr<detail::map_impl>	impl;

public:
    typedef detail::map_impl::off_type	off_type;
    typedef detail::map_impl::size_type	size_type;

    template <class T> class view;

    /// is_open()
    ///
    /// Returns: true if THIS references initialized memory mapped object.

    bool is_open () const { return impl; }

    /// size()
    ///
    /// Returns: size of memory mapped object (size of underlying file).

    off_type size () const { return impl? impl->get_size(): 0; }

    /// close()
    ///
    /// Effects: removes reference to underlying memory mapped object, if any.

    void close () { impl.reset(); }

    /// writeable()
    ///
    /// Returns: true if memory mapped object is open with writing enabled, false
    /// otherwise.
    bool writeable () const { return impl? impl->writeable(): false; }

    /// page_size() and page_mask()
    ///
    /// Return system-dependent virtual page size and corresponding bitwise mask.

    static size_type page_size () { return detail::map_impl::page_size(); }
    static size_type page_mask () { return detail::map_impl::page_mask(); }

protected:
    /// map_base is a base class for memory mapped objects and can be constructed by
    /// ancestors only.

    map_base () : impl() { }

    /// map_base (FILENAME, MODE, SIZE)
    ///
    /// Effects: create memory mapped object on top of the file FILENAME with access
    /// mode MODE.
    /// Throws: sys::generic_error if map cannot be created,
    ///         std::bad_alloc if memory allocation for map failed.

    template <typename CharT>
    map_base (const CharT* filename, mode_t mode, off_type size = 0) : impl()
	{ open (filename, mode, size); }

    /// map_base (HANDLE, MODE, SIZE)
    ///
    /// Effects: create memory mapped object with access mode MODE on top of file
    /// referenced by system handle HANDLE.  HANDLE could be safely used for any i/o
    /// operations (read, write, close etc) after map_base object is created.
    /// Throws: sys::generic_error if map cannot be created,
    ///         std::bad_alloc if memory allocation for map failed.

    map_base (sys::raw_handle handle, mode_t mode, off_type size = 0) : impl()
	{ open (handle, mode, size); }

    map_base (const map_base& other) : impl (other.impl)
        { }

    ~map_base () { }

    map_base& operator= (const map_base& other)
        {
            impl = other.impl;
            return *this;
        }

    template <typename CharT>
    void open (const CharT* filename, mode_t mode, off_type size = 0);
    void open (sys::raw_handle handle, mode_t mode, off_type size = 0);

private:
    /// open_mode (MODE)
    ///
    /// Returns: system-dependent file open mode corresponding to specified map access
    /// mode.

    static io::sys_mode open_mode (mode_t mode)
	{
#ifdef _WIN32
	    return io::sys_mode (write == mode? io::read_write: io::generic_read,
				 io::open_existing);
#else
	    return io::sys_mode (mode == read? O_RDONLY: O_RDWR);
#endif
	}
};

enum write_mode_t {
    writeshare	= write,	// normal read-write mode
    writecopy	= copy,		// copy-on-write mode
};

/// \class sys::mapping::readonly
///
/// class referring to readonly memory mapped object.

class readonly : public map_base
{
public:
    readonly () { }
    template <typename CharT>
    explicit readonly (const CharT* filename, off_type size = 0)
       	: map_base (filename, read, size) { }
    template <typename Ch, typename Tr, typename Al>
    explicit readonly (const basic_string<Ch,Tr,Al>& filename, off_type size = 0)
	: map_base (filename.c_str(), read, size) { }
    explicit readonly (sys::raw_handle handle, off_type size = 0)
       	: map_base (handle, read, size) { }

    template <typename CharT>
    void open (const CharT* filename, off_type size = 0)
       	{ map_base::open (filename, read, size); }

    template <typename Ch, typename Tr, typename Al>
    void open (const basic_string<Ch,Tr,Al>& filename, off_type size = 0)
	{ map_base::open (filename.c_str(), read, size); }

    void open (sys::raw_handle handle, off_type size = 0)
       	{ map_base::open (handle, read, size); }
};

/// \class sys::mapping::readwrite
///
/// class referring to read-write memory mapped object.

class readwrite : public map_base
{
public:
    readwrite () { }

    template <typename CharT>
    explicit readwrite (const CharT* filename, write_mode_t mode = writeshare,
			off_type size = 0)
       	: map_base (filename, mode == writeshare? write: copy, size) { }

    template <typename Ch, typename Tr, typename Al>
    explicit readwrite (const basic_string<Ch,Tr,Al>& filename, write_mode_t mode = writeshare,
		       	off_type size = 0)
       	: map_base (filename.c_str(), mode == writeshare? write: copy, size) { }

    explicit readwrite (sys::raw_handle handle, write_mode_t mode = writeshare,
			off_type size = 0)
       	: map_base (handle, mode == writeshare? write: copy, size) { }

    template <typename CharT>
    void open (const CharT* filename, write_mode_t mode = writeshare, off_type size = 0)
       	{ map_base::open (filename, mode == writeshare? write: copy, size); }

    template <typename Ch, typename Tr, typename Al>
    void open (const basic_string<Ch,Tr,Al>& filename, write_mode_t mode = writeshare,
	       off_type size = 0)
	{ map_base::open (filename.c_str(), mode == writeshare? write: copy, size); }

    void open (sys::raw_handle handle, write_mode_t mode = writeshare, off_type size = 0)
       	{ map_base::open (handle, mode == writeshare? write: copy, size); }
};

/// \class sys::mapping::map_base::view
///
/// base class that maps views of memory mapped object into the address space of the
/// calling process.

template<class T>
class SYSPP_DLLIMPORT map_base::view
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;
    typedef T                   value_type;

protected:
    /// view (MAP, OFFSET, N)
    ///
    /// Effects: creates a view of memory mapped object MAP, starting at offset
    /// OFFSET, containing N objects of type T.  if N is zero, tries to map all
    /// available area.
    /// Throws: std::invalid_argument if MAP does not refer to initialized memory
    ///                               mapped object.
    ///         std::range_error if OFFSET is greater than size of MAP.
    ///         sys::generic_error if some system errors occurs.

    explicit view (map_base& mf, off_type offset = 0, size_type n = 0)
	: map (mf.impl), area (0)
	{ do_remap (offset, n); }

    view (view&& other)
        : map (other.map), area (other.area), msize (other.msize)
        { other.area = 0; }

    template <typename U>
    view (const view<U>& other, off_type offset, size_type n)
        : map (other.map), area (0)
        { do_remap (offset, n); }

    view () : map (0), area (0) { }

    ~view () { if (area) map->unmap ((void*)area, msize*sizeof(T)); }

    template <typename U> friend class view;

public:
    bool is_bound () const { return map; }
    T* get () const { return area; }
    T* data () const { return area; }
    size_type size () const { return msize; }

    view& operator= (view&& other)
        {
            if (area && area != other.area)
                map->unmap ((void*)area, msize*sizeof(T));
            map = other.map;
            area = other.area;
            msize = other.msize;
            other.area = 0;
            return *this;
        }

    T* operator-> () const { return area; }
    T& operator* () const { return *area; }
    T& operator[] (size_t n) const { return area[n]; }

    T* begin () const { return area; }
    T* end   () const { return area+size(); }

    bool sync () { return area? map->sync (area, msize*sizeof(T)): false; }

    void bind (const map_base& mf)
        {
            unmap();
            map = mf.impl;
        }
    template <typename U>
    void bind (const view<U>& other)
        {
            unmap();
            map = other.map;
        }
    void remap (const map_base& mf, off_type offset = 0, size_type n = 0)
	{
            bind (mf);
	    do_remap (offset, n);
	}
    void remap (off_type offset, size_type n)
        {
            unmap();
            do_remap (offset, n);
        }
    void unmap ()
	{
	    if (area)
	    {
		map->unmap ((void*)area, msize*sizeof(T));
		area = 0;
	    }
	}

    off_type max_offset () const { return map->get_size(); }

private:
    void do_remap (off_type offset, size_type n);

    refcount_ptr<detail::map_impl>	map;
    T*		area;	// pointer to the beginning of view address space
    size_type	msize;	// size of view in terms of T objects

    view (const view&);			// not defined
    view& operator= (const view&);	//
};

template <class T>
class SYSPP_DLLIMPORT view : public map_base::view<T>
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;

    view () : map_base::view<T>() { }
    explicit view (readwrite& rwm, off_type offset = 0, size_type n = 0)
	: map_base::view<T> (rwm, offset, n)
	{ }
    view (view&& other) : map_base::view<T> (std::move (other)) { }

    template <typename U>
    view (const view<U>& other, off_type offset, size_type n)
        : map_base::view<T> (other, offset, n)
        { }
};

template <class T>
class SYSPP_DLLIMPORT const_view : public map_base::view<const T>
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;

    const_view () : map_base::view<const T>() { }
    explicit const_view (map_base& map, off_type offset = 0, size_type n = 0)
	: map_base::view<const T> (map, offset, n)
	{ }
    const_view (const_view&& other) : map_base::view<const T> (std::move (other)) { }
    const_view (map_base::view<T>&& other) : map_base::view<const T> (std::move (other)) { }

    template <typename U>
    const_view (const map_base::view<U>& other, off_type offset, size_type n)
        : map_base::view<const T> (other, offset, n)
        { }
};

// --- template methods implementation ---------------------------------------

template <typename CharT> inline void map_base::
open (const CharT* filename, mode_t mode, off_type size)
{
    // this handle automatically closes itself on function exit
    sys::file_handle handle (sys::create_file (filename, open_mode (mode), io::share_default));
    if (!handle) SYS_THROW_FILE_ERROR (filename);
    try {
	open (handle, mode, size);
    }
    catch (generic_error& X)
    {
	throw file_error (X.get_error_code(), filename);
    }
}

template <class T> void map_base::view<T>::
do_remap (off_type offset, size_type n)
{
    assert (0 == area);

    if (!map)
	throw std::invalid_argument ("map_base::view: taking view of an uninitialized map");
    const auto map_size = map->get_size();
    if (sizeof(T) > map_size || offset > map_size-sizeof(T))
	throw std::range_error ("map_base::view: offset exceedes map size");

    size_type byte_size = n*sizeof(T);
    if (!byte_size || off_type (byte_size) > map_size || offset > map_size-byte_size)
	byte_size = static_cast<size_type> (map_size - offset);

    void* v = map->map (offset, byte_size);
    if (!v) SYS_THROW_SYSTEM_ERROR();
    area = static_cast<T*> (v);
    msize = byte_size / sizeof(T);
}

} // namespace mapping

/// \class mapped_file
///
/// the use of mapped_file class is discouraged since there's no means to prevent
/// write to readonly mapped objects.  better use sys::mapping::readwrite,
/// sys::mapping::readonly and desired view classes upon them instead.

class SYSPP_DLLIMPORT mapped_file : public mapping::map_base
{
public:
    mapped_file () { }

    template <typename CharT>
    explicit mapped_file (const CharT* filename, mapping::mode_t mode, off_type size = 0)
       	: map_base (filename, mode, size) { }

    explicit mapped_file (sys::raw_handle handle, mapping::mode_t mode, off_type size = 0)
       	: map_base (handle, mode, size) { }

    template <typename CharT>
    void open (const CharT* filename, mapping::mode_t mode, off_type size = 0)
       	{ map_base::open (filename, mode, size); }

    void open (sys::raw_handle handle, mapping::mode_t mode, off_type size = 0)
       	{ map_base::open (handle, mode, size); }

    template <class T>
    class view;
};

template <class T>
class SYSPP_DLLIMPORT mapped_file::view : public mapping::map_base::view<T>
{
public:
    typedef mapping::map_base::off_type		off_type;
    typedef mapping::map_base::size_type	size_type;

    view () : mapping::map_base::view<T>() { }
    explicit view (mapping::map_base& map, off_type offset = 0, size_type n = 0)
	: mapping::map_base::view<T> (map, offset, n)
	{ }
};

} // namespace sys

#endif /* SYSMEMMAP_H */
