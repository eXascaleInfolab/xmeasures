//! \brief Extrinsic measures evaluation interface.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#ifndef INTERFACE_H
#define INTERFACE_H

#include <unordered_set>
#include <unordered_map>
#include <memory>  // unique_ptr
#include <type_traits>

#include "coding.hpp"  // AggHash
#define INCLUDE_STL_FS
#include "fileio.h"


using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::unique_ptr;
using std::is_integral;
using std::is_pointer;
//using std::enable_if;
using std::enable_if_t;


// Data Types ------------------------------------------------------------------
using Id = uint32_t;  //!< Node id type

//! Accumulated Id type
// Note: Size should a magnitude larger than Id to hold Id*Id
using AccId = uint64_t;

//! Aggregated Hash of the loading cluster member ids
using AggHash = daoc::AggHash<Id, AccId>;

using RawIds = vector<Id>;  //!< Node ids

struct Cluster;

//! Cluster matching counter
//! \note Required only for F1 evaluation
class Counter {
	Cluster*  m_orig;  //!<  Originator cluster
	Id  m_count;  //!<  Occurrences counter
public:
    //! Default constructor
	Counter(): m_orig(nullptr), m_count(0)  {}

    //! \brief Update the counter from the specified origin
    //!
    //! \param orig Cluster*  - counter origin
    //! \return void
	void operator()(Cluster* orig) noexcept
	{
		if(m_orig != orig) {
			m_orig = orig;
			m_count = 0;
		}
		++m_count;
	}

    //! \brief Get counted value
    //!
    //! \return Id  - counted value
	Id operator()() const noexcept  { return m_count; }

    //! \brief Get counter origin
    //!
    //! \return Cluster*  - counter origin
	Cluster* origin() const noexcept  { return m_orig; }

    //! \brief Clear (reset) the counter
	void clear() noexcept
	{
		m_orig = nullptr;
		m_count = 0;
	}
};

using Prob = float;  //!< Probability
using AccProb = double;  //!< Accumulated Probability

struct Cluster {
	RawIds  members;  //!< Node ids, unordered
	Counter  counter;  //!< Cluster matching counter

    //! Default constructor
	Cluster(): members(), counter()  {}

    //! \brief F1 measure
    //!
    //! \param matches Id  - the number of matched members
    //! \param size Id  - size of the matching foreign cluster
    //! \return AccProb  - resulting F1
	AccProb f1(Id matches, Id size) const noexcept
	{
		// F1 = 2 * pr * rc / (pr + rc)
		// pr = m / c1
		// rc = m / c2
		// F1 = 2 * m/c1 * m/c2 / (m/c1 + m/c2) = 2 * m / (c2 + c1)
		return 2 * matches / AccProb(size + members.size());  // E [0, 1]
	}
};

using Clusters = unordered_set<unique_ptr<Cluster>>;  //!< Clusters storage, input collection

using ClusterPtrs = vector<Cluster*>;  //!< Cluster pointers, unordered
using NodeClusters = unordered_map<Id, ClusterPtrs>;  //!< Node to clusters relations

//! Resulting F1_MAH-s for 2 input collections of clusters in a single direction
using F1s = vector<Prob>;

// NMI-related types -----------------------------------------------------------
//! Internal element of the Sparse Matrix with Vector Rows
template<typename Index, typename Value>
struct RowVecItem {
	static_assert(is_integral<Index>::value || is_pointer<Index>::value
		, "RowVecItem, Index should be an integral type");

	using CallT = Index;  //!< Type of the functor call

	Index  pos;  //!< Position (index) in the row
	Value  val;  //!< Target value (payload)

	//! Constructor in case of the simple value
    //!
    //! \param i=Index() Index  - index of value in the row
    //! \param v=Value() Value  - payload value
	template <typename T=Value, enable_if_t<sizeof(T) <= sizeof(void*)>* = nullptr>
	RowVecItem(Index i=Index(), Value v=Value()) noexcept(Value())
	: pos(i), val(v)  {}

	//! Constructor in case of the compound value
    //!
    //! \param i=Index() Index  - index of value in the row
    //! \param v=Value() Value  - payload value
	template <typename T=Value, enable_if_t<(sizeof(T) > sizeof(void*)), bool>* = nullptr>
	RowVecItem(Index i=Index(), Value&& v=Value()) noexcept(Value())
	: pos(i), val(move(v))  {}

    //! \brief Functor (call) operator
    //!
    //! \return CallT  - index of the value
	// Note: required to call obj()
	CallT operator()() const noexcept  { return pos; }

//	// Note: required for the comparison operations with index
//	operator CallT() const noexcept  { return this }
};

//! Row vector for the SparseMatrix
template<typename Index, typename Value>
using SparseMatrixRowVec = vector<RowVecItem<Index, Value>>;

//! Base type of the SparseMatrix (can be unordered_map, map, vector)
template<typename Index, typename Value>
using SparseMatrixBase = unordered_map<Index, SparseMatrixRowVec<Index, Value>>;

//! Sparse Matrix
template<typename Index, typename Value>
struct SparseMatrix: SparseMatrixBase<Index, Value> {
	using IndexT = Index;  //!< Indexes type, integral
	using ValueT = Value;  //!< Value type
	using BaseT = SparseMatrixBase<IndexT, ValueT>;  //!< SparseMatrixBase type
	using RowT = typename BaseT::mapped_type;  //!< Matrix row type
	//! Matrix row element type, which contains the value and might have
	//! additional attributes
	using RowItemT = typename RowT::value_type;

    //! \brief Default constructor
    //!
    //! \param rows=0 Id  - initial number of rows
	SparseMatrix(Id rows=0);

    //! \brief Access specified element inserting it if not exists
    //!
    //! \param i Index  - row index
    //! \param j Index  - column index
    //! \return Value& operator  - value of the element to be set
	Value& operator ()(Index i, Index j);

    //! \brief Access specified element without bounds checking
    //! \note fast, but unsafe
    //!
    //! \param i Index  - row index
    //! \param j Index  - column index
    //! \return Value& operator  - value of the element
	template <typename T=Value, enable_if_t<sizeof(T) <= sizeof(void*)>* = nullptr>
	Value operator ()(Index i, Index j) const noexcept; //  { return this->at(i) }

    //! \brief Access specified element without bounds checking
    //! \note fast, but unsafe
    //!
    //! \param i Index  - row index
    //! \param j Index  - column index
    //! \return Value& operator  - value of the element
	template <typename T=Value, enable_if_t<(sizeof(T) > sizeof(void*)), bool>* = nullptr>
	const Value& operator ()(Index i, Index j) const noexcept; //  { return this->at(i) }

    //! \brief Access specified element checking the bounds
    //!
    //! \param i Index  - row index
    //! \param j Index  - column index
    //! \return Value& operator  - value of the element
	template <typename T=Value, enable_if_t<sizeof(T) <= sizeof(void*)>* = nullptr>
	Value at(Index i, Index j); //  { return this->at(i) }

    //! \brief Access specified element checking the bounds
    //!
    //! \param i Index  - row index
    //! \param j Index  - column index
    //! \return Value& operator  - value of the element
	template <typename T=Value, enable_if_t<(sizeof(T) > sizeof(void*)), bool>* = nullptr>
	const Value& at(Index i, Index j); //  { return this->at(i) }

	using BaseT::at;  //!< Provide direct access to the matrix row
};

// Collection ------------------------------------------------------------------
//! Collection describing cluster-node relations
class Collection {
	Clusters  m_cls;  //!< Clusters
	NodeClusters  m_ndcs;  //!< Node clusters relations
protected:
    //! Default constructor
	Collection(): m_cls(), m_ndcs()  {}
public:
    //! \brief Clusters count
    //!
    //! \return Id  - the number of clusters in the collection
	Id clusters() const noexcept  { return m_cls.size(); }

    //! \brief Nodes count
    //!
    //! \return Id  - the number of nodes in the collection
	Id nodes() const noexcept  { return m_ndcs.size(); }

	//! \brief Load collection from the CNL file
	//! \pre All clusters in the file are expected to be unique and not validated for
	//! the mutual match
	//!
	//! \param filename const char*  - name of the input file
    //! \param ahash=nullptr AggHash*  - resulting aggregated hash of the loaded
    //! member ids if not nullptr
	//! \param membership=1 float  - expected membership of nodes, >0, typically >= 1
    //! \return bool  - the collection is loaded successfully
	static Collection load(const char* filename, AggHash* ahash=nullptr, float membership=1);

	//! \brief F1 Max Average Harmonic Mean evaluation considering overlaps,
	//! multi-resolution and possibly unequal node base
	//! \note Undirected (symmetric) evaluation
	//!
	//! \param cn1 const Collection&  - first collection
	//! \param cn2 const Collection&  - second collection
    //! \param weighted=false bool  - weighted average by cluster size
	//! \return Prob  - resulting F1_MAH
	static Prob f1mah(const Collection& cn1, const Collection& cn2, bool weighted=false);

	//! \brief NMI evaluation considering overlaps, multi-resolution and possibly
	//! unequal node base
	//! \note Undirected (symmetric) evaluation
	//!
	//! \param cn1 const Collection&  - first collection
	//! \param cn2 const Collection&  - second collection
    //! \param expbase=true bool  - use ln (exp base) or log2 (Shannon entropy, bits)
    //! for the information measuring
	//! \return Prob  - resulting F1_MAH
	static Prob nmi(const Collection& cn1, const Collection& cn2, bool expbase=true);
protected:
    //! \brief F1 Max Average relative to the specified collection FROM this one
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const Collection&  - collection to compare with
    //! \param weighted=false bool  - weighted average by cluster size
    //! \return AccProb  - resulting max average f1 from this collection
    //! to the specified one (DIRECTED)
	inline AccProb f1MaxAvg(const Collection& cn, bool weighted=false) const;

    //! \brief Max F1 for each cluster
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const Collection&  - collection to compare with
    //! \return F1s - resulting max F1 for each member node
	F1s clsF1Max(const Collection& cn) const;

	//! \brief NMI evaluation considering overlaps, multi-resolution and possibly
	//! unequal node base
	//! \note Undirected (symmetric) evaluation
    //!
    //! \param cn const Collection&  - collection to compare with
    //! \param expbase=true bool  - use ln (exp base) or log2 (Shannon entropy, bits)
    //! for the information measuring
    //! \return Prob  - resulting NMI
	Prob nmi(const Collection& cn, bool expbase=true) const;
};

#endif // INTERFACE_H
