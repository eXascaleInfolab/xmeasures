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

#include <unordered_map>
#include <memory>  // unique_ptr
#include <string>
#include <type_traits>
#include <limits>
#if VALIDATE >= 1
#include <stdexcept>
#endif // VALIDATE

#define INCLUDE_STL_FS
#include "fileio.hpp"


using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::unique_ptr;
using std::string;
using std::is_integral;
using std::is_pointer;
using std::is_floating_point;
using std::is_arithmetic;
using std::is_same;
//using std::enable_if;
using std::enable_if_t;
using std::conditional_t;
using std::numeric_limits;
#if VALIDATE >= 1
using std::domain_error;
#if VALIDATE >= 2
using std::invalid_argument;
#endif // VALIDATE
#endif // VALIDATE

// Data Types ------------------------------------------------------------------
using Id = uint32_t;  //!< Node id type

//! Accumulated Id type
// Note: Size should a magnitude larger than Id to hold Id*Id
using AccId = uint64_t;

//! Aggregated Hash of the loading cluster member ids
using AggHash = daoc::AggHash<Id, AccId>;

using RawIds = vector<Id>;  //!< Node ids, unordered

template <typename Count>
struct Cluster;

//! Cluster matching counter
//! \note Required only for F1 evaluation
//! \tparam Count  - arithmetic counting type
template <typename Count>
class Counter {
public:
	static_assert(is_arithmetic<Count>::value
		, "Counter(), Count should be an arithmetic type");
	using CountT = Count;  //!< Count type, arithmetic
	using ClusterT = Cluster<Count>;
private:
	ClusterT*  m_orig;  //!<  Originator cluster
	CountT  m_count;  //!<  Occurrences counter, <= members size
public:

    //! Default constructor
	Counter(): m_orig(nullptr), m_count(0)  {}

    //! \brief Update the counter from the specified origin
    //!
    //! \param orig ClusterT*  - counter origin
    //! \return void
	void operator()(ClusterT* orig, Count cont)
#if VALIDATE < 2
	noexcept
#endif // VALIDATE
	{
		if(m_orig != orig) {
			m_orig = orig;
			m_count = 0;
		}
		if(is_integral<CountT>::value)
			++m_count;
		else {
			static_assert(!is_floating_point<CountT>::value || sizeof(m_count) >= sizeof(double)
				, "operator(), types validation failed");
#if VALIDATE >= 2
			if(cont <= 0 || cont > 1)
				throw invalid_argument("operator(), cont should E (0, 1]\n");
#endif // VALIDATE
			m_count += cont;
		}
	}

    //! \brief Get counted value
    //!
    //! \return CountT  - counted value
	CountT operator()() const noexcept  { return m_count; }

    //! \brief Get counter origin
    //!
    //! \return ClusterT*  - counter origin
	ClusterT* origin() const noexcept  { return m_orig; }

    //! \brief Clear (reset) the counter
	void clear() noexcept
	{
		m_orig = nullptr;
		m_count = 0;
	}
};

using Prob = float;  //!< Probability
using AccProb = double;  //!< Accumulated Probability

//! Cluster
//! \tparam Count  - nodes contribution counter type
template <typename Count>
struct Cluster {
	static_assert(is_arithmetic<Count>::value
		, "Counter(), Count should be an arithmetic type");
	using CountT = Count;  //!< Count type, arithmetic

	RawIds  members;  //!< Node ids, unordered
	// Note: used by F1 only and always
	Counter<Count>  counter;  //!< Cluster matching counter
	////! Accumulated contribution
	//using AccCont = conditional_t<m_overlaps, Count, AccId>;
	//!< Contribution from members
	// Note: used only in case of overlaps by all measures, and by NMI only
	// in case of multiple resolutions
	Count  mbscont;
	static_assert(!is_floating_point<Count>::value || sizeof(mbscont) >= sizeof(double)
		, "operator(), types validation failed");

    //! Default constructor
	Cluster();

    //! \brief F1 measure
    //! \pre Clusters should be valid, i.e. non-empty
    //!
    //! \param matches Count  - the number of matched members
    //! \param capacity Count  - contributions capacity of the matching foreign cluster
    //! \return AccProb  - resulting F1
	AccProb f1(Count matches, Count capacity) const
#if VALIDATE < 2
	noexcept
#endif // VALIDATE
	{
		// F1 = 2 * pr * rc / (pr + rc)
		// pr = m / c1
		// rc = m / c2
		// F1 = 2 * m/c1 * m/c2 / (m/c1 + m/c2) = 2 * m / (c2 + c1)
		// ATTENTION: F1 compares clusters per-pair, so it is much simpler and has another
		// semantics of contribution for the multi-resolution case
		const Count  contrib = is_floating_point<Count>::value ? cont() : members.size();
#if VALIDATE >= 2
		if(matches < 0 || daoc::less<conditional_t<is_floating_point<Count>::value
		, Prob, Count>>(capacity, matches) || contrib <= 0)
			throw invalid_argument(string("f1(), both clusters should be non-empty, matches: ")
				.append(std::to_string(matches)).append(", capacity: ").append(std::to_string(capacity))
				.append(", contrib: ").append(std::to_string(contrib)));
#endif // VALIDATE
		return 2 * matches / AccProb(capacity + contrib);  // E [0, 1]
		// Note that partial probability (non-normalized to the remained matches,
		// it says only how far this match from the full match) of the match is:
		// P = AccProb(matches * matches) / AccProb(size * members.size()),
		// where nodes contribution instead of the size should be use for overlaps.
		// The probability is more discriminative than F1 for high values
	}

    //! \brief Partial probability of the match (non-normalized to the other matches)
    //! \pre Clusters should be valid, i.e. non-empty
    //!
    //! \param matches Count  - the number of matched members
    //! \param capacity Count  - contributions capacity of the matching foreign cluster
    //! \return AccProb  - resulting probability
	AccProb pprob(Count matches, Count capacity) const
#if VALIDATE < 2
	noexcept
#endif // VALIDATE
	{
		// P = P1 * P2 = m/n1 * m/n2 = m*m / (n1*n2),
		// where nodes contribution instead of the size should be use for overlaps.
		// ATTENTION: F1 compares clusters per-pair, so it is much simpler and has another
		// semantics of contribution for the multi-resolution case comparing to NMI
		// that also uses cont()
		constexpr bool  floating = is_floating_point<Count>::value;
		const Count  contrib = floating ? cont() : members.size();
#if VALIDATE >= 2
		if(matches < 0 || daoc::less<conditional_t<floating, Prob, Count>>
		(capacity, matches) || contrib <= 0)
			throw invalid_argument(string("pprob(), both clusters should be non-empty, matches: ")
				.append(std::to_string(matches)).append(", capacity: ").append(std::to_string(capacity))
				.append(", contrib: ").append(std::to_string(contrib)));
#endif // VALIDATE
		return floating ? static_cast<AccProb>(matches) * matches / (static_cast<AccProb>(capacity) * contrib)
			: static_cast<AccProb>(static_cast<AccId>(matches) * matches)
				/ (static_cast<AccId>(capacity) * contrib);  // E [0, 1]
	}

    //! \brief Cluster members contribution
    //!
    //! \return Count  - total contribution from the members
	Count cont() const noexcept
	{
//		return is_same<decltype(mbscont), EmptyStub>::value ? members.size() : mbscont;
		return mbscont;
	}
};

//! Automatic storage for the Cluster;
//! \tparam Count  - arithmetic counting type
template <typename Count>
using ClusterHolder = unique_ptr<Cluster<Count>>;

//! Automatic storage for the Cluster;
//! \tparam Count  - arithmetic counting type
template <typename Count>
using Clusters = unordered_set<ClusterHolder<Count>>;

//! Cluster pointers, unordered
//! \tparam Count  - arithmetic counting type
template <typename Count>
using ClusterPtrs = vector<Cluster<Count>*>;

//! Node to clusters relations
//! \tparam Count  - arithmetic counting type
template <typename Count>
using NodeClusters = unordered_map<Id, ClusterPtrs<Count>>;

//! Resulting greatest matches for 2 input collections of clusters in a single direction
using Probs = vector<Prob>;

// F1-related types -----------------------------------------------------------
using F1Base = uint8_t;

//! \brief F1 kind
enum struct F1: F1Base {
	//! Note initialized
	NONE = 0,
	//! Harmonic mean of the [weighted] average of the greatest (maximal) match
	//! by partial probabilities
	PARTPROB,
	//! Harmonic mean of the [weighted] average of the greatest (maximal) match by F1s
	HARMONIC,
	//! Arithmetic mean (average) of the [weighted] average of the greatest (maximal)
	//! match by F1s, i.e. F1-Score
	STANDARD
};

// NMI-related types -----------------------------------------------------------
//! Internal element of the Sparse Matrix with Vector Rows
//! \tparam Index  - index (of the column) in the row
//! \tparam Value  - value type
template <typename Index, typename Value>
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
template <typename Index, typename Value>
using SparseMatrixRowVec = vector<RowVecItem<Index, Value>>;

//! Base type of the SparseMatrix (can be unordered_map, map, vector)
template <typename Index, typename Value>
using SparseMatrixBase = unordered_map<Index, SparseMatrixRowVec<Index, Value>>;

//! Sparse Matrix
//! \tparam Index  - index type
//! \tparam Value  - value type
template <typename Index, typename Value>
struct SparseMatrix: SparseMatrixBase<Index, Value> {
	static_assert((is_integral<Index>::value || is_pointer<Index>::value)
		&& is_arithmetic<Value>::value, "SparseMatrix(), invalid parameter types");

	using IndexT = Index;  //!< Indexes type, integral
	using ValueT = Value;  //!< Value type, arithmetic
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

//using EvalBase = uint8_t;  //!< Base type for the Evaluation
//
////! \brief Evaluation type
//enum struct Evaluation: EvalBase {
//	NONE = 0,
////	HARD = 0
//	MULTIRES = 1,  //!< Multi-resolution non-overlapping clusters, compatible with hard partitioning
//	OVERLAPPING = 2,  //!< Overlapping clusters, compatible with hard partitioning
//	MULRES_OVP = 3  //!< Multi-resolution clusters with possible overlaps on each resolution level
//};
//
////! \brief Convert Evaluation to string
////! \relates Evaluation
////!
////! \param flag Evaluation  - the flag to be converted
////! \param bitstr=false bool  - convert to bits string or to Evaluation captions
////! \return string  - resulting flag as a string
//string to_string(Evaluation eval, bool bitstr=false);

struct RawNmi {
	Prob  mi;  //!< Mutual information of two collections
	Prob  h1;  //!< Information content of the 1-st collection
	Prob  h2;  //!< Information content of the 2-nd collection
	//Evaluation  eval;  //!< Evaluation type

	static_assert(is_floating_point<Prob>::value, "RawNmi, Prob should be a floating point type");
	RawNmi() noexcept: mi(0), h1(numeric_limits<Prob>::quiet_NaN())
		, h2(numeric_limits<Prob>::quiet_NaN())  {}

	void operator() (Prob mutinf, Prob cn1h, Prob cn2h) noexcept
	{
		mi = mutinf;
		h1 = cn1h;
		h2 = cn2h;
	};
};

// Collection ------------------------------------------------------------------
//! Node base interface
struct NodeBaseI {
    //! \brief Default virtual destructor
	virtual ~NodeBaseI()=default;

    //! \brief Whether the node base is actual (non-empty)
    //!
    //! \return bool  - the node base is non-empty
	operator bool() const noexcept  { return ndsnum(); };

    //! \brief The number of nodes
    //!
    //! \return Id  - the number of nodes in the collection
	virtual Id ndsnum() const noexcept = 0;

    //! \brief Whether exists the specified node
    //!
    //! \param nid  - node id
    //! \return bool  - specified node id exists
	virtual bool nodeExists(Id nid) const noexcept = 0;
};

//! Unique ids (node ids)
using UniqIds = unordered_set<Id>;

//! Node base interface
struct NodeBase: UniqIds, NodeBaseI {
	//! \copydoc NodeBaseI::nodeExists(Id nid) const noexcept
	Id ndsnum() const noexcept  { return size(); }

	//! \copydoc NodeBaseI::nodeExists(Id nid) const noexcept
	bool nodeExists(Id nid) const noexcept  { return count(nid); }

	//! \brief Load nodes from the CNL file with optional filtering by the cluster size
	//!
	//! \param filename const char*  - name of the input file
    //! \param ahash=nullptr AggHash*  - resulting aggregated hash of the loaded
    //! node ids if not nullptr
	//! \param membership=1 float  - expected membership of the nodes, >0, typically >= 1.
	//! Used only for the node container preallocation to estimate the number of nodes
	//! if not specified in the file header
	//! \param cmin=0 size_t  - min allowed cluster size
	//! \param cmax=0 size_t  - max allowed cluster size, 0 means any size
    //! \param verbose=false bool  - print intermediate results to the stdout
    //! \return bool  - the collection is loaded successfully
	static NodeBase load(const char* filename, float membership=1
		, AggHash* ahash=nullptr, size_t cmin=0, size_t cmax=0, bool verbose=false);
};

//! Collection describing cluster-node relations
//! \tparam Count  - arithmetic counting type
template <typename Count>
class Collection: public NodeBaseI {
public:
	using CollectionT = Collection<Count>;
	//! Overlaps / multi-resolutions evaluation flag
	constexpr static bool  m_overlaps = is_floating_point<Count>::value;
	//! Accumulated contribution
	using AccCont = conditional_t<m_overlaps, Count, AccId>;
	//! Clusters matching matrix
	using ClustersMatching = SparseMatrix<Cluster<Count>*, AccCont>;  // Used only for NMI
private:
	Clusters<Count>  m_cls;  //!< Clusters
	NodeClusters<Count>  m_ndcs;  //!< Node clusters relations
	size_t  m_ndshash;  //!< Nodes hash (of unique node ids only, not all members), 0 means was not evaluated
	//mutable bool  m_dirty;  //!< The cluster members contribution is not zero (should be reseted on reprocessing)
	//! Sum of contributions of all members in each cluster
	mutable AccCont  m_contsum;  // Used by NMI only
protected:
    //! Default constructor
	Collection(): m_cls(), m_ndcs(), m_ndshash(0), m_contsum(0)  {}  //, m_dirty(false)  {}
public:
    //! \brief The number of clusters
    //!
    //! \return Id  - the number of clusters in the collection
	Id clsnum() const noexcept  { return m_cls.size(); }

    //! \brief The number of nodes
    //!
    //! \return Id  - the number of nodes in the collection
	Id ndsnum() const noexcept  { return m_ndcs.size(); }

	//! \copydoc NodeBaseI::nodeExists(Id nid) const noexcept
	bool nodeExists(Id nid) const noexcept  { return m_ndcs.count(nid); }

	//! \brief Load collection from the CNL file
	//! \pre All clusters in the file are expected to be unique and not validated for
	//! the mutual match
	//!
	//! \param filename const char*  - name of the input file
	//! \param membership=1 float  - expected membership of the nodes, >0, typically >= 1.
	//! Used only for the node container preallocation to estimate the number of nodes
	//! if not specified in the file header
    //! \param ahash=nullptr AggHash*  - resulting hash of the loaded
    //! member ids base (unique ids only are hashed, not all ids) if not nullptr
	//! \param const nodebase=nullptr NodeBaseI*  - node base to filter-out nodes if required
	//! \param verbose=false bool  - print the number of loaded nodes to the stdout
    //! \return CollectionT  - the collection is loaded successfully
	static CollectionT load(const char* filename, float membership=1
		, AggHash* ahash=nullptr, const NodeBaseI* nodebase=nullptr
		, bool verbose=false);

	//! \brief Specified F1 evaluation of the Greatest (Max) Match for the
	//! multi-resolution clustering with possibly unequal node base
	//!
	//! Supported F1 measures are F1p <= F1h <= F1s, where:
	//! - F1p  - Harmonic mean of the [weighted] average of partial probabilities,
	//! 	the most discriminative and satisfies the largest number of the Formal
	//! 	Constraints (homogeneity, completeness, rag bag,  size/quantity, balance);
	//! - F1h  - Harmonic mean of the [weighted] average of F1s;
	//! - F1s  - Standard F1-Score, i.e. arithmetic mean (average) of the [weighted]
	//! 	average of F1s, the least discriminative and satisfies the lowest number
	//! 	of the Formal Constraints.
	//!
	//! of the Greatest (Max) Match [Weighted] Average Harmonic Mean evaluation
	//! \note Undirected (symmetric) evaluation
	//!
	//! \param cn1 const CollectionT&  - first collection
	//! \param cn2 const CollectionT&  - second collection
    //! \param kind F1  - kind of F1 to be evaluated
    //! \param weighted=true bool  - weighted average by cluster size or unweighted
    //! \param verbose=false bool  - print intermediate results to the stdout
	//! \return Prob  - resulting F1_gm
	static Prob f1(const CollectionT& cn1, const CollectionT& cn2, F1 kind
		, bool weighted=true, bool verbose=false);

	//! \brief NMI evaluation
	//! \note Undirected (symmetric) evaluation
	//!
	//! \param cn1 const CollectionT&  - first collection
	//! \param cn2 const CollectionT&  - second collection
    //! \param expbase=false bool  - use ln (exp base) or log2 (Shannon entropy, bits)
    //! for the information measuring
    //! \param verbose=false bool  - perform additional verification and print details
	//! \return RawNmi  - resulting NMI
	static RawNmi nmi(const CollectionT& cn1, const CollectionT& cn2, bool expbase=false
		, bool verbose=false);
protected:
	// F1-related functions ----------------------------------------------------
    //! \brief Average of the maximal matches (by F1 or partial probabilities)
    //! relative to the specified collection FROM this one
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions. Small collection relative to the average
    //! or average relative to huge might yield best matching F1 equal to 1, but
    //! then the back match should be small.
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const CollectionT&  - collection to compare with
    //! \param weighted bool  - weighted average by cluster size
    //! \param prob bool  - evaluate partial probability instead of F1
    //! \return AccProb  - resulting max average match value from this collection
    //! to the specified one (DIRECTED)
	inline AccProb avggms(const CollectionT& cn, bool weighted, bool prob) const;

    //! \brief Greatest (Max) matching value (F1 or partial probability) for each cluster
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const CollectionT&  - collection to compare with
    //! \param prob bool  - evaluate partial probability instead of F1
    //! \return Probs - resulting max F1 or partial probability for each member node
	Probs gmatches(const CollectionT& cn, bool prob) const;

	// NMI-related functions ---------------------------------------------------
	//! \brief NMI evaluation considering overlaps, multi-resolution and possibly
	//! unequal node base
	//! \note Undirected (symmetric) evaluation
    //!
    //! \param cn const CollectionT&  - collection to compare with
    //! \param expbase bool  - use ln (exp base) or log2 (Shannon entropy, bits)
    //! for the information measuring
    //! \return RawNmi  - resulting NMI
	RawNmi nmi(const CollectionT& cn, bool expbase) const;

    //! \brief Clear contributions in each cluster and optionally
    //! evaluate the clusters matching
    //!
    //! \param cn const CollectionT&  - foreign collection to be processed with this one
    //! \param[out] clsmm=nullptr ClustersMatchingT*  - clusters matching matrix to be filled
    //! \return AccCont  - sum of all values of the clsmm matrix if specified
	AccCont evalconts(const CollectionT& cn, ClustersMatching* clsmm=nullptr) const;

    //! \brief Clear contributions in each cluster
    //!
    //! \return void
	void clearconts() const noexcept;
};

#endif // INTERFACE_H
