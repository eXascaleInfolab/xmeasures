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

#define INCLUDE_STL_FS
#include "fileio.h"


using std::unordered_set;
using std::unordered_map;
using std::unique_ptr;


// Data Types ------------------------------------------------------------------
using Id = uint32_t;

using RawIds = vector<Id>;  //!< Node ids

//using RawCollection = vector<RawIds>;  //!< Clusters of nodes

struct Cluster;

//! Cluster matching counter
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

    //! \brief F1 renormalized relative to the current cluster
    //!
    //! \param matches Id  - the number of matched members
    //! \param size Id  - size of the matching foreign cluster
    //! \return AccProb  - resulting renormalized relative f1
	AccProb rf1(Id matches, Id size) const noexcept
	{
		// F1 = 2 * pr * rc / (pr + rc)
		// pr = m / c1
		// rc = m / c2
		// F1 = 2 * m/c1 * m/c2 / (m/c1 + m/c2)
		// Relative F1 = m * m/c2 / (m/c1 + m/c2) = m/c2 / (1/c1 + 1/c2)
		// = m*c1 / c2 + c1
		return matches / AccProb(size + members.size()) * size;  // E [0, 1)
	}
};

using Clusters = unordered_set<unique_ptr<Cluster>>;  //!< Clusters storage, input collection

using ClusterPtrs = vector<Cluster*>;  //!< Cluster pointers, ordered
using NodeClusters = unordered_map<Id, ClusterPtrs>;  //!< Node to clusters relations

//! Resulting F1_MAH-s for 2 input collections of clusters in a single direction
using F1s = vector<Prob>;

//! Collection describing cluster-node relations
class Collection {
	Clusters  m_cls;  //!< Clusters
	NodeClusters  m_ndcs;  //!< Node clusters relations
protected:
    //! Default constructor
	Collection(): m_cls(), m_ndcs()  {}

    //! \brief Max F1 for each member node
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const Collection&  - collection to compare with
    //! \return F1s - resulting max F1 for each member node
	F1s mbsF1Max(const Collection& cn) const;

    //! \brief F1 Max Average relative to the specified collection FROM this one
    //! \note External cn collection can have unequal node base and overlapping
    //! clusters on multiple resolutions
    //! \attention Directed (non-symmetric) evaluation
    //!
    //! \param cn const Collection&  - collection to compare with
    //! \return AccProb  - resulting max average f1 from this collection
    //! to the specified one (DIRECTED)
	inline AccProb f1MaxAvg(const Collection& cn) const;
public:
	//! \brief F1 Max Average Harmonic Mean considering overlaps,
	//! multi-resolution and possibly unequal node base
	//! \note Undirected (symmetric) evaluation
	//!
	//! \param cn1 const Collection&  - first collection
	//! \param cn2 const Collection&  - second collection
	//! \return Prob  - resulting F1_MAH
	static Prob f1mah(const Collection& cn1, const Collection& cn2);

	//! \brief Load collection from the CNL file
	//! \pre All clusters in the file are expected to be unique and not validated for
	//! the mutual match
	//!
	//! \param filename const char*  - name of the input file
	//! \param membership=1 float  - expected membership of nodes, >0, typically >= 1
    //! \return bool  - the collection is loaded successfully
	static Collection load(const char* filename, float membership=1);
};

// Function Interfaces ---------------------------------------------------------
//! \brief NMI evaluation considering overlaps, multi-resolution and possibly
//! unequal node base
//!
//! \param cn1 const Collection&  - first collection
//! \param cn2 const Collection&  - second collection
//! \return float  - resulting NMI
float evalNmi(const Collection& cn1, const Collection& cn2);

#endif // INTERFACE_H
