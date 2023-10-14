/* -*- C++ -*-
 * File:        type_traits.hpp
 * Created:     Tue Jul 24 04:15:38 2007
 * Description: type traits template classes.
 *
 * $Id$
 */

#ifndef EXT_TYPE_TRAITS_HPP
#define EXT_TYPE_TRAITS_HPP

#include <boost/type_traits.hpp>
#include <boost/call_traits.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/cstdint.hpp>
#include <iterator>	// for std::iterator_traits
#include <limits>

namespace ext {

namespace mpl = boost::mpl;

// ---------------------------------------------------------------------------
// Utility templates that help to specialize library template members.

// iterator_category(ITER)
// Returns: object of the type corresponding to category of iterator ITER.

template <typename Iter>
inline typename std::iterator_traits<Iter>::iterator_category
iterator_category (const Iter&)
{
    return typename std::iterator_traits<Iter>::iterator_category();
}

struct true_type {};
struct false_type {};

namespace detail {
    template <bool x> struct boolean_c { typedef false_type type; };
    template <> struct boolean_c<true> { typedef true_type type;  };
} // namespace detail

template <class T> struct type_traits {
    typedef typename detail::boolean_c<
	    boost::is_integral<T>::value>::type	is_integral;
    // Evaluates to true only if T is a cv-qualified integral type.

    typedef typename detail::boolean_c<
	    boost::is_arithmetic<T>::value>::type is_arithmetic;
    // Evaluates to true only if T is a cv-qualified arithmetic type.
    // That is either an integral or floating point type.
};

namespace detail {

struct arithmetic_type_tag {};
struct integral_type_tag {};
struct floating_type_tag {};
struct signed_int_type_tag {};
struct unsigned_int_type_tag {};

// detail::iterator_category<T>
// yields to category tag of the iterator type T.  if T is not an iterator, yields
// compile-time error.

template <typename T> struct iterator_category {
    typedef typename std::iterator_traits<T>::iterator_category type;
};

// detail::type_tag<T>
// if T is integral, yields to detail::integral_type_tag, otherwise to
// std::iterator_traits<T>::iterator_category

template <class T> struct type_tag {
    typedef typename
    mpl::eval_if<boost::is_integral<T>,
	mpl::identity<integral_type_tag>,
	iterator_category<T>
    >::type type;
};

// detail::int_type_tag<T>
// if T is signed integer type, yields to detail::signed_int_type_tag,
// if T is unsigned integer type, yields to detail::unsigned_int_type_tag,
// otherwise yields to false_type

template <class T> struct int_type_tag {
    typedef typename
    mpl::if_<boost::is_integral<T>,
        mpl::if_<boost::is_signed<T>,
            signed_int_type_tag,
            unsigned_int_type_tag>,
        false_type
    >:: type type;
};

// detail::non_const_param<T>::type
// yields to non-const reference to T if it is class/struct type and to
// 'boost::call_traits<T>::param_type' otherwise.

template <typename T>
struct non_const_param {
    typedef typename
    mpl::if_<boost::is_class<T>,
	T&,
	typename boost::call_traits<T>::param_type
    >::type type;
};

// detail::cv_qualify<T,U>::type
// resulting type is an U with all cv-qualifiers that T has.

template <typename T, typename U>
struct cv_qualify {
    typedef typename
    mpl::eval_if<boost::is_const<T>,
	mpl::eval_if<boost::is_volatile<T>,
	    boost::add_cv<U>,
	    boost::add_const<U>
	>,
	mpl::eval_if<boost::is_volatile<T>,
	    boost::add_volatile<U>,
	    mpl::identity<U>
	>
    >::type type;
};

// detail::char_upcast<T>::type
// if T is 'char' or 'wchar_t' (possibly cv-qualified), yields to 'int', preserving
// signed/unsigned and cv-qualifiers.  otherwise, yields to T.

template <typename T> struct char_upcast_impl {
    typedef typename
    mpl::eval_if<boost::is_same<T,char>,	  mpl::identity<int>,
    mpl::eval_if<boost::is_same<T,unsigned char>, mpl::identity<unsigned int>,
    mpl::eval_if<boost::is_same<T,signed char>,   mpl::identity<signed int>,
    mpl::eval_if<boost::is_same<T,wchar_t>,	  mpl::identity<int>,
    mpl::identity<T>
    > > > >::type type;
};

template <typename T> struct char_upcast {
    typedef typename cv_qualify<T,
	typename char_upcast_impl<typename boost::remove_cv<T>::type>::type
    >::type type;
};

// detail::float_cast<Src,Dst>::type
// if Src is a (possibly cv-qualified) floating-point type, yields to Src, otherwise
// yields Dst, preserving cv-qualifiers.

template <typename Src, typename Dst> struct float_cast {
    typedef typename
    mpl::if_<boost::is_float<typename boost::remove_reference<Src>::type >,
	Src,
	Dst
//	cv_qualify<Src, Dst>
    >::type type;
};

// detail::is_negative<T> (X)
// Returns: true if T is a signed type and X is less than zero, false otherwise.

template <bool is_signed> struct neg_helper {
    template <typename T> static bool is_negative (T) { return false; }
};

template <> struct neg_helper<true> {
    template <typename T> static bool is_negative (T x) { return x < 0; }
};

template <typename T>
inline bool is_negative (T x)
{
    typedef typename boost::remove_cv<
	    typename boost::remove_reference<T>::type
	    >::type value_type;
    return neg_helper<std::numeric_limits<value_type>::is_signed>
	::template is_negative<T> (x);
}

// detail::void_ptr_cast<T>::type
// if T is a cv-pointer type, yields to 'cv-void*', otherwise yields to T

template <typename T>
struct void_ptr_cast_impl {
    typedef typename
    mpl::eval_if<boost::is_pointer<T>,
	boost::add_pointer<typename cv_qualify<
	    typename boost::remove_pointer<T>::type, void>::type >,
	mpl::identity<T>
    >::type type;
};

template <typename T> struct void_ptr_cast {
    typedef typename cv_qualify<T,
	typename void_ptr_cast_impl<typename boost::remove_cv<T>::type>::type
    >::type type;
};

// detail::nearest_int<T>
// yields to integer type enough to hold object of type T

template <bool ulong_is_enough>
struct nearest_int_impl
{
    typedef boost::uint32_t type;
};

template <>
struct nearest_int_impl<false>
{
    typedef boost::uint64_t type;
};

template <typename T>
struct nearest_int
{
    typedef typename nearest_int_impl<sizeof(T) <= sizeof(boost::uint32_t)>::type type;
};

template <typename T>
struct pointer_to_int
{
    typedef typename boost::remove_cv<
	    typename boost::remove_reference<T>::type>::type arg_type;
    static T do_cast (T p) { return p; }
};

template <typename T>
struct pointer_to_int<T*>
{
    typedef typename nearest_int<T*>::type type;
    static type do_cast (T* p) { return reinterpret_cast<type> (p); }
};

template <typename T>
struct integer_cast
{
    typedef typename boost::remove_reference<T>::type		arg_type;
    typedef typename detail::char_upcast<arg_type>::type	value_type;

    static value_type do_cast (T x) { return static_cast<value_type> (x); }
};

template <typename T>
struct integer_cast<T*>
{
    typedef T*							arg_type;
    typedef typename detail::nearest_int<arg_type>::type	value_type;

    static value_type do_cast (T* x) { return reinterpret_cast<value_type> (x); }
};

} // namespace detail

} // namespace ext

#endif /* EXT_TYPE_TRAITS_HPP */
