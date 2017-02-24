//! \brief The Dao of Clustering library - Robust & Fine-grained Deterministic Clustering for Large Networks
//! 	Definition of basic common operations.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2015-08-06

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <cstddef>  // ptrdiff_t
#include <cmath>  // fabs(), isinf()
#include <limits>  // Type limits
#include <type_traits>  // conditional, type check
#include <cassert>
#include <utility>  // declval, forward, move
#include <iterator>  // iterator_traits

#include "macrodef.h"

#if TRACE >= 2
#include <cstdio>
#endif // TRACE

namespace daoc {

// Internal MACROSES:
//	- INSORTED_NONUNIQUE  - insorted() does not validate that the inserting
//	element is not present
//
// NOTE: undefined maro definition is interpreted as having value 0

using std::ptrdiff_t;
using std::numeric_limits;
using std::iterator_traits;
using std::is_same;
using std::is_floating_point;
using std::is_integral;
using std::is_reference;
using std::is_base_of;
using std::declval;
using std::enable_if_t;


// Compile time tracing functions ---------------------------------------------
//! \brief Trace specified type in compile time
//!
//! \return void
template <typename T>
constexpr void traceType()
{
	static_assert(T()._, "traceType() termination");
	// Or use RTTI typeid()
}

// Accessory type identifiers -------------------------------------------------
//! \brief Whether the type is an iterator of the specified category
//! \tparam T  - evaluating type
//! \tparam Category=std::input_iterator_tag  - target base category to be checked
//!
//! \return constexpr bool  - the type is iterator
template <typename T, typename Category=std::input_iterator_tag
	, enable_if_t<is_base_of<Category, typename iterator_traits<T>::iterator_category>::value>* = nullptr>
constexpr bool is_iterator() noexcept
{ return true; }

template <typename T, typename Category=std::input_iterator_tag
	, enable_if_t<!is_base_of<Category, typename iterator_traits<T>::iterator_category>::value, bool>* = nullptr>
constexpr bool is_iterator() noexcept
{ return false; }

template <typename T, typename Category=std::input_iterator_tag>
constexpr bool is_iterator_v = is_iterator<T, Category>();


//! \brief Whether the type is an unordered associative container
//! \tparam T  - evaluating type
template <typename T, typename Accessory=void>
struct is_hashContainer: std::false_type  {};

template <typename T>
struct is_hashContainer<T, enable_if_t<std::is_object<typename T::hasher>::value>>
: std::true_type  {};

template <typename T>
constexpr bool is_hashContainer_v = is_hashContainer<T>::value;

// Accessory math functions ---------------------------------------------------
//! \brief Precision limit (error) for the specified floating type
template <typename ValT>
constexpr ValT precision_limit() noexcept
{
	static_assert(is_floating_point<ValT>::value, "precision_limit() type should be fractional");
	// Note: much lower epsilon does not impact convergence,
	// but usage of sqrt(epsilon) allows to skip the size in comparison functions
//	return numeric_limits<ValT>::epsilon() * 2;  // Last digit should be precise
	return sqrt(numeric_limits<ValT>::epsilon());  // Note: sqrt(x^a) = x^(a/2) to filter out accumulation errors
}

//! \brief Strict less for floating point numbers
//!
//! \param a ValT  - val1
//! \param b ValT  - val2
//! \param size=1 size_t  - average number of elements in a and b
//! 	to consider accumulation error
//! \return bool  - val1 < val2
template <typename ValT, enable_if_t<is_floating_point<ValT>::value, float>* = nullptr>
constexpr bool less(const ValT a, const ValT b=ValT(0), const size_t size=1) noexcept
{
	static_assert(is_floating_point<ValT>::value, "less(), value type should be fractional");
	// a < b   <= For x > 0:  a + x < b =>  For y > 0:  a - b + y * abs(a+b) < 0
	// ATTENTION:
	// - "1 +" is required to handle accumulation error correctly for float numbers < 1
	// - "precision_limit" must be the first multiplier to have correct evaluation for huge values
	// Note: it is more strict than a < b, fabs(a + b) is fine also when the signs are distinct
	return a - b + precision_limit<ValT>() * size * (1 + fabs(a + b)) < 0;
}

//! \brief Strict less for integral numbers
template <typename ValT, enable_if_t<is_integral<ValT>::value, int>* = nullptr>
constexpr bool less(const ValT a, const ValT b=ValT(0), const size_t size=1) noexcept
{
	static_assert(is_integral<ValT>::value, "less(), value type should be integral");
	return a < b;
}

//! \brief Check equality of 2 floating point numbers
//!
//! \param a ValT  - val1
//! \param b ValT  - val2
//! \param size=1 size_t  - average number of elements in a and b
//! 	to consider accumulation error
//! \return bool  - whether val1 ~= val2
template <typename ValT, enable_if_t<is_floating_point<ValT>::value, float>* = nullptr>
//#if VALIDATE <= 1
constexpr
//#else
//inline
//#endif // VALIDATE
bool equal(const ValT a, const ValT b=ValT(0), const size_t size=1) noexcept
{
	static_assert(is_floating_point<ValT>::value, "equal(), value type should be fractional");
//#if VALIDATE >= 2
//	// Allow stand-alone nodes
//	//assert(size >= 1 && "equal(), size should be positive");
//#if TRACE >= 3
//	fprintf(stderr, "equal()  a: %g, b: %g, cnt: %lu, precLim: %G (from %G)"
//		"\n\tres: %d, val: %g, lim: %g (lmul: %g)\n"
//		, a, b, size, precision_limit<ValT>(), numeric_limits<ValT>::epsilon()
//		, fabs(a - b) <= precision_limit<ValT>() * size * (1 + fabs(a) + fabs(b))
//		, fabs(a - b), precision_limit<ValT>() * size * (1 + fabs(a) + fabs(b))
//		, (1 + fabs(a) + fabs(b)));
//#endif // TRACE_EXTRA
//#endif // VALIDATE
	// ~ a == b with strictly defined equality margin
	// ATTENTION:
	// - "1 +" is required to handle accumulation error correctly for float numbers < 1
	// - "fabs(a + b)" is not appropriate here in case of a = -b && b > 0  => fabs(a) + fabs(b)
	// - "precision_limit" must be the first multiplier to have correct evaluation for huge values
	//// - "size + 1" is used to handle case of size = 0 (stand-alone nodes) correctly
	return fabs(a - b) <= precision_limit<ValT>() * size * (1 + fabs(a) + fabs(b));
}

//! \brief Strict equal for integral numbers
template <typename ValT, enable_if_t<is_integral<ValT>::value, int>* = nullptr>
constexpr bool equal(const ValT a, const ValT b=ValT(0), const size_t size=1) noexcept
{
	static_assert(is_integral<ValT>::value, "equal(), value type should be integral");
	return a == b;
}

//! \brief Divide and round the result
//!
//! \param a ValAT  - dividend
//! \param b ValBT  - divisor
//! \return ValAT  - rounded quotient
template <typename ValAT, typename ValBT>
inline ValAT rdiv(const ValAT a, const ValBT b)
{
	static_assert(is_integral<ValAT>::value, "rdiv(), dividend should be integral");
	assert(b && "rdiv(), b must be != 0");
	ValAT  base = a / b;
	base += ((a - base * b) << 1) / b;
	return base;
}

// Comparison functions -------------------------------------------------------
//! Comparison function for the arguments
// Note: ptrdiff_t casting is required otherwise binary_find() is error prone
template <typename ValT>
constexpr bool cmpBase(const ValT a, const ValT b) noexcept
{
//	using ValT = common_type<Val1T, Val2T>::type;
//	// Note: scalar includes pointer
//	static_assert((is_scalar<ValT>::value || is_reference<ValT>::value)
	static_assert(!is_floating_point<ValT>::value
		&& (sizeof(ValT) <= sizeof(void*)  || is_reference<ValT>::value)
		, "cmpBase(), arguments should be integral and either small objects or references");
	return a < b;
}

//! Comparison function for objects sorting by dest attribute
//! \note Uses cmpBase()
template <typename T>
constexpr bool cmpDest(const T& a, const T& b) noexcept
{ return cmpBase(a.dest, b.dest); }

// Binary search --------------------------------------------------------------
constexpr int  BINSEARCH_MARGIN = 9;  // Min size of an array when bin search is faster than a linear scan

//! \brief Direct value comparison (binary find callback)
//! 	Note: Items in the container are assumed to be ordered in acs order
//! \attention must be synced with cmpBase()
//!
//! \param cv T  - container value
//! \param v T  - control value
//! \return ptrdiff_t  - comparison result: cv - v
template <typename T>  //, enable_if_t<sizeof(T) <= sizeof(ptrdiff_t)>* = nullptr>
constexpr ptrdiff_t bsVal(const T cv, const T v) noexcept
// ATTENTION: without casting difference will be incorrect for the types with size
// different from ptrdiff_t and for the unsigned types
{
//	using ValT = common_type<Val1T, Val2T>::type;
	static_assert(!is_floating_point<T>::value
		&& sizeof(T) <= sizeof(ptrdiff_t)
		, "bsVal(), argument type constraints failed");
	return reinterpret_cast<ptrdiff_t>(cv) - reinterpret_cast<ptrdiff_t>(v);
}

//template <typename T, enable_if_t<(sizeof(T) > sizeof(ptrdiff_t))>* = nullptr>
//constexpr ptrdiff_t bsVal(T& cv, T& v)  // ATTENTION: non-const ref is used to
//// ATTENTION: without casting difference will be incorrect for the types with size
//// different from ptrdiff_t and for the unsigned types
//{
//	// ATTENTION: dereference is used to treat iterators correctly
//	return reinterpret_cast<ptrdiff_t>(&*cv) - reinterpret_cast<ptrdiff_t>(&*v);
//}

////! \brief Dereferenced value address comparison (binary find callback)
////! 	Useful to process a container of iterators
////! 	Note: Items in the container are assumed to be ordered in acs order
////! \attention must be synced with cmpBase()
////!
////! \param cv T  - container value
////! \param v T  - control value
////! \return ptrdiff_t  - comparison result: cv - v
//template <typename T>  //, enable_if_t<sizeof(T) <= sizeof(ptrdiff_t)>* = nullptr>
//constexpr ptrdiff_t bsDerefAddr(const T& cv, const T& v)
//// ATTENTION: without casting difference will be incorrect for the types with size
//// different from ptrdiff_t and for the unsigned types
//{
//	static_assert(sizeof(T) > sizeof(ptrdiff_t)
//		, "bsDerefAddr(), heavy dereferencable type is expected");
//	return bsVal(&*cv, &*v);
//}

//! Binary sort comparison function for objects sorting by dest attribute
//! \note Uses bsVal()
//!
//! \param cv T  - container value
//! \param v T  - control value
//! \return ptrdiff_t  - comparison result
template <typename T>
constexpr ptrdiff_t bsDest(const T& a, const T& b) noexcept
{ return bsVal(a.dest, b.dest); }

//! \brief Elements dest comparison (binary find callback)
//! \note Items in the container are assumed to be ordered in acs order by bsVal().
//!
//! \tparam T  - container type
//! \param obj typename T::const_reference  - object
//! \param dest const typename T::value_type::DestT*  - dest attribute
//! \return ptrdiff_t  - comparison result
template <typename T, enable_if_t<(sizeof(typename T::value_type::DestT) > sizeof(void*))>* = nullptr>
constexpr ptrdiff_t bsObjsDest(typename T::const_reference obj
	, const typename T::value_type::DestT* dest) noexcept
{ return bsVal<const typename T::value_type::DestT*>(obj.dest, dest); }

//! \copydoc bsObjsDest<typename T, enable_if_t<(sizeof(typename T::value_type::DestT) > sizeof(void*))>* = nullptr>
template <typename T, enable_if_t<sizeof(typename T::value_type::DestT) <= sizeof(void*), bool>* = nullptr>
constexpr ptrdiff_t bsObjsDest(typename T::const_reference obj
	, const typename T::value_type::DestT dest) noexcept
{ return bsVal<const typename T::value_type::DestT>(obj.dest, dest); }

//! \brief Elements operator() comparison (binary find callback)
//! \note Items in the container are assumed to be ordered in acs order by bsVal().
//!
//! \tparam T  - container type
//! \param obj typename T::const_reference  - object
//! \param val const typename T::value_type::CallT  - comparing value
//! \return ptrdiff_t  - comparison result: cv - v
template <typename T>
inline ptrdiff_t bsObjsOp(typename T::const_reference obj
	, const typename T::value_type::CallT val) noexcept(typename T::value_type()())
// ATTENTION: without casting difference will be incorrect for the types with size
// different from ptrdiff_t and for the unsigned types
{
	static_assert(sizeof(typename T::value_type::CallT) <= sizeof(ptrdiff_t)
		, "bsObjsOp(), argument type constraints failed");
	return bsVal<const typename T::value_type::CallT>(obj(), val);
}

//! \brief Element operator() comparison (binary find callback)
//! \note Items in the container are assumed to be ordered in acs order by bsVal().
//!
//! \tparam T  - object type
//! \param obj const T&  - object
//! \param val const typename T::CallT  - comparing value
//! \return ptrdiff_t  - comparison result: cv - v
template <typename T>
inline ptrdiff_t bsObjOp(const T& obj, const typename T::CallT val) noexcept
// ATTENTION: without casting difference will be incorrect for the types with size
// different from ptrdiff_t and for the unsigned types
{
	static_assert(sizeof(typename T::CallT) <= sizeof(ptrdiff_t)
		, "bsObjOp(), argument type constraints failed");
	return bsVal<const typename T::CallT>(obj(), val);
}

////! \brief Element dest comparison (binary find callback)
////! \note Items in the container are assumed to be ordered in acs order by bsVal().
////!
////! \param obj typename T::const_reference  - object
////! \param dest const typename T::value_type::DestT*  - dest attribute
////! \return ptrdiff_t  - comparison result
//template <typename T>
//constexpr ptrdiff_t bsObjDest(T obj
//	, const typename T::DestT* dest)
//{ return bsVal<const typename T::DestT*>(obj.dest, dest); }

//! \brief Binary sort comparison function for objects sorting by get() member function
//! \note Uses bsVal()
//!
//! \param cv T  - container value
//! \param v T  - control value
//! \return ptrdiff_t  - comparison result
template <typename T>
constexpr ptrdiff_t bsObjGetF(const T& cv, const T& v) noexcept(noexcept(cv.get()) && noexcept(v.get()))
{ return bsVal(cv.get(), v.get()); }

//! \brief Lambda function of struct comparison by the attr
//! \pre The attribute should be non-floating point object having the size not larger
//! 	 than the address type size
//!
//! \param T  - type of the struct to be compared
//! \param attr  - comparison field (attribute)
#define BS_STRUCT(T, attr)  \
	[](const T& cv, const T& v) noexcept -> ptrdiff_t \
	{  \
		using TAttr = decltype(v.attr);  \
		static_assert(!is_floating_point<TAttr>::value  \
			&& sizeof(TAttr) <= sizeof(ptrdiff_t)  \
			, "bs#T(), argument type constraints failed");  \
		return reinterpret_cast<ptrdiff_t>(cv.attr) - reinterpret_cast<ptrdiff_t>(v.attr);  \
	}
// 	~ bsVal<decltype(v.attr)>(cv.attr, v.attr);  // But bsVal complicates compile-time error analysis

//! \brief Lambda function of struct binary search the attr
//! \pre The attribute should be non-floating point object having the size not larger
//! 	 than the address type size
//!
//! \param T  - type of the struct
//! \param attr  - comparison field (attribute)
#define BS_OBJ_ATTR(T, attr)  \
	[](const T& el, const decltype(declval<T>().attr) ea) noexcept -> ptrdiff_t \
	{  \
		using TAttr = decltype(el.attr);  \
		static_assert(!is_floating_point<TAttr>::value  \
			&& sizeof(TAttr) <= sizeof(ptrdiff_t)  \
			, "bs#T#attr(), argument type constraints failed");  \
		return reinterpret_cast<ptrdiff_t>(el.attr) - reinterpret_cast<ptrdiff_t>(ea);  \
	}

//! \brief Binary search in the ordered range with random iterators
//! 	for the closest following elemnt
//! \pre The elements are ordered by bsVal()
//! \post Returned iterator always has not less value than required
//!
//! \tparam T  - value type
//! \tparam RandIT  - random iterator
//! \tparam CompareF  - NONSTANDARD comparison function:
//! 		int cmp(const RandIT::reference iterVal, const T& val);
//! 			returns negative if iterVal < val, 0 if iterVal == val
//! 			and positive otherwise
//! \return random iterator of the value position if exists, otherwise
//! 	the iterator on the following value / end
template <typename T, typename RandIT, typename CompareF>
RandIT binary_ifind(RandIT begin, const RandIT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<RandIT>::value_type>)
#if VALIDATE < 2
	noexcept(CompareF())
#endif // VALIDATE
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "binary_ifind(), T should not increase the address type size");
	static_assert(is_same<decltype(cmp(*begin, val)), decltype(bsVal(nullptr, nullptr))>::value
		, "binary_ifind(), cmp() must return the same type as bsVal()");
	static_assert(is_iterator<RandIT, std::random_access_iterator_tag>()
		, "binary_ifind(), RandIT must be a random iterator type");
	RandIT iend = end;
	for(RandIT  pos = begin + (iend - begin) / 2; pos != iend
	; pos = begin + (iend - begin) / 2) {
		int  cres = cmp(*pos, val);

		if(!((cres < 0 && ((begin = pos + 1), true)) || (cres > 0 && ((iend = pos), true))))
			return pos;
	}
#if VALIDATE >= 2
#if TRACE >= 3
	if(!(iend == end || cmp(*iend, val) <= 0)) {
		// Note: iend != end is always true here
		fprintf(stderr, "  >>>>> binary_ifind(), iend: %s, val: %s. Container: "
			, iend->idstr().c_str(), val->idstr().c_str());
		for(auto ic = begin; ic != end; ++ic)
			fprintf(stderr, " %s", ic->idstr().c_str());
		fputs("\n", stderr);
	}
#endif // TRACE
	assert((iend == end || cmp(*iend, val) >= 0)
	&& "binary_ifind(), iterator verification failed");
#endif // VALIDATE
	return iend;
}

//! Binary search in the ordered range with random iterators
template <typename T, typename RandIT, typename CompareF>
inline RandIT binary_find(RandIT begin, const RandIT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<RandIT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "binary_find(), T should not increase the address type size");
	begin = binary_ifind(begin, end, val, cmp);
	return begin != end && !cmp(*begin, val) ? begin : end;
}

// Linear search --------------------------------------------------------------
//! \brief Linear search in the specified range for the closest following element
//! \pre The elements are ordered by bsVal()
//!
//! \tparam T  - value type
//! \tparam IT  - iterator
//! \tparam CompareF  - NONSTANDARD comparison function:
//! 		int cmp(const IT::reference iterVal, const T& val);
//! 			returns negative if iterVal < val, 0 if iterVal == val
//! 			and positive otherwise
//! \return iterator of the value position or end
template <typename T, typename IT, typename CompareF
	, enable_if_t<sizeof(T) <= sizeof(void*)>* = nullptr>
IT linear_ifind(IT begin, const IT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<IT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "linear_ifind(), T should not increase the address type size");
	static_assert(is_same<decltype(cmp(*begin, val)), decltype(bsVal(nullptr, nullptr))>::value
		, "linear_ifind(), cmp() must return the same type as bsVal()");
	static_assert(is_iterator<IT>(), "linear_ifind(), IT must be a forward iterator type");
	while(begin != end && cmp(*begin, val) < 0)
		++begin;
	return begin;
}

template <typename T, typename IT, typename CompareF
	, enable_if_t<(sizeof(T) > sizeof(void*)), bool>* = nullptr>
IT linear_ifind(IT begin, const IT end, const T& val
	, CompareF cmp=bsVal<typename iterator_traits<IT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) > sizeof(void*)
		, "linear_ifind(), compound type T is expected");
	static_assert(is_same<decltype(cmp(*begin, val)), decltype(bsVal(nullptr, nullptr))>::value
		, "linear_ifind(), cmp() must return the same type as bsVal()");
	static_assert(is_iterator<IT>(), "linear_ifind(), IT must be a forward iterator type");
	while(begin != end && cmp(*begin, val) < 0)
		++begin;
	return begin;
}

//! Linear search in the specified range
template <typename T, typename IT, typename CompareF>
inline IT linear_find(IT begin, const IT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<IT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "linear_find(), T should not increase the address type size");
	begin = linear_ifind(begin, end, val, cmp);
	return begin != end && !cmp(*begin, val) ? begin : end;
}

//! Linear index find in the sorted dataset
template <typename T, typename Container, typename CompareF>
inline typename Container::iterator linear_ifind(Container& cnt, const T val
	, CompareF cmp=bsVal<typename Container::value_type>) noexcept(CompareF())
{
	static_assert(is_iterator<typename Container::iterator, std::random_access_iterator_tag>()
		, "linear_ifind(), Container iterator must be a random iterator type");
	return linear_ifind(cnt.begin(), cnt.end(), val, cmp);
}

//! Fast index find in the sorted dataset using either binary or linear search
template <typename T, typename RandIT, typename CompareF>
inline RandIT fast_ifind(RandIT begin, const RandIT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<RandIT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "fast_ifind(), T should not increase the address type size");
	static_assert(is_iterator<RandIT, std::random_access_iterator_tag>()
		, "fast_ifind(), RandIT must be a random iterator type");
	return (end - begin) < BINSEARCH_MARGIN
		? linear_ifind(begin, end, val, cmp)
		: binary_ifind(begin, end, val, cmp);
}

//! Fast index find in the sorted dataset using either binary or linear search
template <typename T, typename Container, typename CompareF>
inline typename Container::iterator fast_ifind(Container& cnt, const T val
	, CompareF cmp=bsVal<typename Container::value_type>) noexcept(CompareF())
{
	static_assert(is_iterator<typename Container::iterator, std::random_access_iterator_tag>()
		, "fast_ifind(), Container iterator must be a random iterator type");
	return fast_ifind(cnt.begin(), cnt.end(), val, cmp);
}

//! Fast find in the sorted dataset using either binary or linear search
template <typename T, typename RandIT, typename CompareF>
inline RandIT fast_find(RandIT begin, const RandIT end, const T val
	, CompareF cmp=bsVal<typename iterator_traits<RandIT>::value_type>) noexcept(CompareF())
{
	static_assert(sizeof(T) <= sizeof(void*)
		, "fast_find(), T should not increase the address type size");
	static_assert(is_iterator<RandIT, std::random_access_iterator_tag>()
		, "fast_find(), RandIT must be a random iterator type");
	return (end - begin) < BINSEARCH_MARGIN
		? linear_find(begin, end, val, cmp)
		: binary_find(begin, end, val, cmp);
}

//! Fast find in the sorted dataset using either binary or linear search
template <typename T, typename Container, typename CompareF>
inline typename Container::iterator fast_find(Container& cnt, const T val
	, CompareF cmp=bsVal<typename Container::value_type>) noexcept(CompareF())
{
	static_assert(is_iterator<typename Container::iterator, std::random_access_iterator_tag>()
		, "fast_find(), Container iterator must be a random iterator type");
	return fast_find(cnt.begin(), cnt.end(), val, cmp);
}

//! Fast find in the sorted dataset using either binary or linear search
template <typename T, typename Container, typename CompareF>
inline typename Container::const_iterator fast_find(const Container& cnt, const T val
	, CompareF cmp=bsVal<typename Container::value_type>) noexcept(CompareF())
{
	static_assert(is_iterator<typename Container::const_iterator, std::random_access_iterator_tag>()
		, "fast_find(), Container iterator must be a random iterator type");
	return fast_find(cnt.begin(), cnt.end(), val, cmp);
}

// Operations with containers -------------------------------------------------
////! \brief Extends ordered nonempty elements
////! \pre els should be not empty and el should not be already present in els
////! \post iel points to the element FOLLOWING the inserted one after the execution
////!
////! \param els ContainerT&  - elements to be extended considering the ordering
//////! \param iel typename ContainerT::iterator&  - begin iterator in the els to
//////! 	speedup incremental inserts.
//////! 	Points to the elements FOLLOWING the inserted one after the execution.
////! \param el typename ContainerT::value_type  - element to be added to els
////! \param ContainerT::value_type> CompareF cmp=bsVal  - comparison function
////! 		int cmp(const ContainerT::value_type& iterVal
////!				, const ContainerT::value_type& val);
////!
////! 			returns negative if iterVal < val, 0 if iterVal == val
////! 			and positive otherwise
////! \return void
//template <typename ContainerT, typename CompareF>
//void addSorted(ContainerT& els  //, typename ContainerT::iterator& iel
//	, const typename ContainerT::value_type el
//	, CompareF cmp=bsVal<typename ContainerT::value_type>)
//{
//	static_assert(sizeof(typename ContainerT::value_type) <= sizeof(void*)
//		, "addSorted(), value_type should not increase the address type size");
//	assert(!els.empty() && "addSorted(), els should be not empty");
//
//	auto iel = els.begin();
//	iel = cmp(*iel, el) < 0 ? fast_ifind(++iel, els.end(), el, cmp) : iel;
//	assert((iel == els.end() || cmp(*iel, el) != 0) && "addSorted(), elements must be unique");
//	els.insert(iel, el);
////	iel = els.insert(iel, el);
////	++iel;  // Note: required to speedup incremental inserts
//}

//! \brief Check whether the items are sorted (ordered by the cmp func)
//!
//! \tparam IterT  - Iterator type
//!
//! \param begin IterT  - begin iterator of the checking range
//! \param end const IterT&  - end iterator of the checking range
//! \param cmp=bsVal CompareF  - comparison function
//! \param unique=true bool  - all items must be unique or allow duplicates
//! \return bool  - the items are ordered by cmp func
template <typename IterT, typename CompareF>
bool sorted(IterT begin, const IterT end
	, CompareF cmp=bsVal<typename iterator_traits<IterT>::value_type>, bool unique=true)
#if VALIDATE < 2
	noexcept(CompareF())
#endif // VALIDATE
{
	static_assert(is_same<decltype(cmp(*begin, *begin)), decltype(bsVal(nullptr, nullptr))>::value
		, "sorted(), cmp() must return the same type as bsVal()");
	static_assert(is_iterator<IterT>(), "sorted(), IterT must be a forward iterator type");
#if VALIDATE >= 2
	assert(begin != end && "sorted(), empty container is being checked");
#endif // VALIDATE
	if(begin == end)
		return true;
	// The container is sorted if it empty or has only one item
	while(++begin != end) {
		auto res = cmp(*begin, *(begin - 1));
		if(res < 0 || (res == 0 && unique))
			return false;
	}
	return true;
}

//! \brief Index of the specified element in the sorted elements
//! \pre Elements are ordered and unique
//! \note If not INSORTED_NONUNIQUE and VALIDATE >= 1 then the elements are validated
//! 	to not contain the searching el
//!
//! \tparam ContainerT  - container type
//! \tparam ItemT  - anchor type
//! \tparam CompareF  - type of the binary search comparison function
//!
//! \param els ContainerT&  - elements to search at
//! \param el const ItemT  - anchor whose position is being defined
//! \param cmp=bsVal<ItemT> CompareF  - comparison function
//! \return typename ContainerT::iterator  - iterator to els where dest should be inserted/emplaced
template <typename ContainerT, typename ItemT, typename CompareF>
typename ContainerT::iterator insorted(ContainerT& els, const ItemT el, CompareF cmp=bsVal<ItemT>)
#if VALIDATE < 3 && (!defined(INSORTED_NONUNIQUE) || VALIDATE < 1)
	noexcept(CompareF())
#endif // VALIDATE
{
	static_assert(sizeof(ItemT) <= sizeof(void*)
		, "insorted(), ItemT should not increase the address type size");
#if VALIDATE >= 3
	assert(sorted(els.begin(), els.end(), cmp) && "insorted(), els should be sorted");
#endif // VALIDATE
	auto iel = els.begin();
	if(!els.empty() && cmp(*iel, el) < 0)
		iel = fast_ifind(++iel, els.end(), el, cmp);
#if VALIDATE >= 1
#if TRACE >= 2
	if(!(iel == els.end() || cmp(*iel, el) != 0))
		fprintf(stderr, "  >>>>>>> insorted(), #%u (%p) is already among %lu members\n"
			, el->id, el, els.size());
#endif // TRACE
#ifndef INSORTED_NONUNIQUE
	assert((iel == els.end() || cmp(*iel, el) != 0) && "insorted(), elements must be unique");
#endif // INSORTED_NONUNIQUE
#endif // VALIDATE
	return iel;
}

//! \brief Index of the specified element in the SMALL number of sorted elements
//! \pre Elements are ordered, unique and does not contain the searching el
//! 	The number of elements is small (usually up to BINSEARCH_MARGIN)
//!
//! \tparam ContainerT  - container type
//! \tparam ItemT  - anchor type
//! \tparam CompareF  - type of the binary search comparison function
//!
//! \param els ContainerT&  - elements to search at
//! \param el const ItemT  - anchor whose position is being defined
//! \param cmp=bsVal<ItemT> CompareF  - comparison function
//! \return typename ContainerT::iterator  - iterator to els where dest should be inserted/emplaced
template <typename ContainerT, typename ItemT, typename CompareF>
inline typename ContainerT::iterator insortedLight(ContainerT& els, const ItemT el
	, CompareF cmp=bsVal<ItemT>)
#if !defined(INSORTED_NONUNIQUE) || VALIDATE < 1
	noexcept(CompareF())
#endif // VALIDATE
{
	static_assert(sizeof(ItemT) <= sizeof(void*)
		, "insortedLight(), ItemT should not increase the address type size");
//#if VALIDATE >= 3
//	assert(sorted(els.begin(), els.end(), cmp) && "insorted(), els should be not sorted");  // Note: issues occur in case of cmp is bsObjsDest
//#endif // VALIDATE
	auto iel = els.begin();
	if(!els.empty() && cmp(*iel, el) < 0)
		iel = linear_ifind(++iel, els.end(), el, cmp);
#if VALIDATE >= 1
#if TRACE >= 2
	if(!(iel == els.end() || cmp(*iel, el) != 0))
		fprintf(stderr, "  >>>>>>> insortedLight(), #%u (%p) is already among %lu members\n"
			, el->id, el, els.size());
#endif // TRACE
#ifndef INSORTED_NONUNIQUE
	assert((iel == els.end() || cmp(*iel, el) != 0) && "insortedLight(), elements must be unique");
#endif // INSORTED_NONUNIQUE
#endif // VALIDATE
	return iel;
}

// Accessory Types declaration ------------------------------------------------
////! Value Statistics
//template <typename ValT>
//class ValStat {
//	ValT  m_sum;  // Sum of values
//	ValT  m_sum2;  // Sum of values squares
//	Id  m_num;  // Number of values
//	ValT  m_min;
//	ValT  m_max;
//	bool  m_fixed;  // Whether statistic accumulation is finalized
//
//	constexpr static ValT  VALUE_MIN = numeric_limits<ValT>::lowest();
//	constexpr static ValT  VALUE_MAX = numeric_limits<ValT>::max();
//public:
//	//! Value for the noninitialized elements
//	constexpr static ValT  VALUE_NONE = numeric_limits<ValT>::lowest();
//	static_assert(!is_same<ValT, AccWeight>::value || VALUE_NONE == ACCWEIGHT_NONE
//		, "ValStat, VALUE_NONE must be synced with ACCWEIGHT_NONE");
//
//	//! \brief Constart new Value Statistics
//	ValStat()
//	: m_sum(0), m_sum2(0), m_num(0), m_min(VALUE_MAX), m_max(VALUE_MIN)
//		, m_fixed(false)  {}
//
//	//! \brief Add subsequent value to the statistics
//	//!
//	//! \param val ValT  - value to be added
//	//! \return void
//	void add(ValT val);
//
//	//! \brief Reset statistics
//	//!
//	//! \return void
//	void reset() {
//		m_sum = m_sum2 = 0;
//		m_num = 0;
//		m_min = VALUE_MAX;
//		m_max = VALUE_MIN;
//		m_fixed = false;
//	}
//
//	//! \brief Finalize statistics accumulation before reading
//	//! \note MUST be called ONCE after the statistics accumulation and
//	//! 	before the values reading
//	//!
//	//! \return void
//	void finalize() {
//		if(m_fixed)
//			throw domain_error("finalize(), repeated finalization is performed\n");
//		m_fixed = true;
//	}
//
//	//! \brief Whether the statistics is evaluated or nothing was processed
//	//!
//	//! \return bool  - the statistics is evaluated
//	bool evaluated() const {
//		return m_min != VALUE_MAX
//			|| m_max != VALUE_MIN;
//	}
//
//	//! \brief Obtain the Mean value
//	//! \pre finalize() must be called after the statistics accumulation and
//	//! 	before the values reading
//	//!
//	//! \return ValT  - statistical Mean
//	ValT mean() const  {
//		if(!m_fixed)
//			throw domain_error("mean(), finalization has not been performed\n");
//		return m_num ? m_sum / m_num : VALUE_NONE;
//	}
//
//	ValT sum() const  {
//		if(!m_fixed)
//			throw domain_error("sum(), finalization has not been performed\n");
//		return m_num ? m_sum : VALUE_NONE;
//	}
//
//	ValT sum2() const  {
//		if(!m_fixed)
//			throw domain_error("sum2(), finalization has not been performed\n");
//		return m_num ? m_sum2 : VALUE_NONE;
//	}
//
//	ValT num() const  {
//		if(!m_fixed)
//			throw domain_error("num(), finalization has not been performed\n");
//		return m_num;
//	}
//
//	ValT min() const  {
//		if(!m_fixed)
//			throw domain_error("min(), finalization has not been performed\n");
//		return m_num ? m_min : VALUE_NONE;
//	}
//
//	ValT max() const  {
//		if(!m_fixed)
//			throw domain_error("max(), finalization has not been performed\n");
//		return m_num ? m_max : VALUE_NONE;
//	}
//};

//// Ids related accessory means
// ATTENTION: AccId depends on Id, currently Id is limited up to 32 bits
//using AccId = uint64_t;  //!< Accumulated Id
//
////! IdSums
//struct IdSums {
//	Id  num;  //!< number of ids
//	AccId  sum;  //!< Sum of Ids
//	AccId  sum2;  //!< Sum2 of ids
//
//	IdSums()
//	: num(0), sum(0), sum2(0)  {}
//
//    //! \brief Accumulate subsequent id
//    //!
//    //! \param id Id  - id to be added
//    //! \return void
//	void add(Id id) {
//		++num;
//		sum += id;
//		sum2 += id*id;
//	}
//
//	bool operator<(const IdSums& idsm) const {
//		return num < idsm.num || (num == idsm.num
//			&& (sum < idsm.sum || (sum == idsm.sum && sum2 < idsm.sum2)));
//	}
//
//	bool operator!=(const IdSums& idsm) const {
//		return num != idsm.num || sum != idsm.sum || sum2 != idsm.sum2;
//	}
//};

}  // daoc

#endif // OPERATIONS_H
