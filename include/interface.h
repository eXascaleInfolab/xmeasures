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

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>  // unique_ptr


// Global MACROSES:
//	- HEAVY_VALIDATION  - use alternative evaluations to validate results
//		- 0  - turn off heavy validation
//		- 1  - default value for the heavy validation
//		- 2  - extra heavy validation (might duplicate already performed heavy validation)
//		- 3  - cross validation of functions (executed on each call, but only once is enough)
//
//	- TRACE, TRACE_EXTRA  - detailed tracing under debug (trace nodes weights)
//		- 0  - turn off the tracing
//		- 1  - brief tracing that can be used in release to show warnings, etc.
//		- 2  - detailed tracing for DEBUG
//		- 3  - extra detailed tracing
//
// NOTE: undefined maro definition is interpreted as having value 0

#ifndef TRACE
#ifdef DEBUG
	#define TRACE 2
#elif !defined(NDEBUG)  // RELEASE, !NDEBUG
	#define TRACE 1
//#else  // RELEASE, NDEBUG
//	#define TRACE 0
#endif // DEBUG
#endif // TRACE

#ifndef HEAVY_VALIDATION
#ifdef DEBUG
	#define HEAVY_VALIDATION 2
#elif !defined(NDEBUG)  // RELEASE, !NDEBUG
	#define HEAVY_VALIDATION 1
//#else  // ELEASE, NDEBUG
//	#define HEAVY_VALIDATION 0
#endif // DEBUG
#endif // HEAVY_VALIDATION

using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::unique_ptr;


// Data Types ------------------------------------------------------------------
using Id = uint32_t;

using RawIds = vector<Id>;
using RawCollection = vector<RawIds>;  //!< Clusters of nodes

struct Cluster;

//! Cluster matching counter
struct Counter {
	Cluster*  orig;  //!<  Originator cluster
	Id  count;  //!<  Occurrences counter

	Counter(): orig(nullptr), count(0)  {}
};

struct Cluster {
	RawIds  members;  //!< ORDERED node ids
	Counter  counter;

//	Cluster(): members(), counter()  {}
};

using Clusters = unordered_set<unique_ptr<Cluster>>;  //!< Clusters storage, input collection

using ClusterPtrs = vector<Cluster*>;
using NodeClusters = unordered_map<Id, ClusterPtrs>;  //!< Node to clusters relations

using F1s = vector<float>;  //!< Resulting F1s for the collection of clusters

// Function Interfaces ---------------------------------------------------------
//! \brief Load clustering from the file
//!
//! \param filename const char*  - name of the input file
//! \param membership=1 float  - membership of nodes, >0, typically >= 1
//! \return Clusters  - loaded clusters
Clusters loadClustering(const char* filename, float membership=1);

//! \brief F1 Max Average Harmonic Mean evaluation  considering overlaps,
//! multi-resolution and possibly unequal node base
//!
//! \param cls1 const Clusters&  - first clustering
//! \param cls2 const Clusters&  - second clustering
//! \return float  - resulting F1_MAH
float evalF1(const Clusters& cls1, const Clusters& cls2);

//! \brief NMI evaluation considering overlaps, multi-resolution and possibly
//! unequal node base
//!
//! \param cls1 const Clusters&
//! \param cls2 const Clusters&
//! \return float  - resulting NMI
float evalNmi(const Clusters& cls1, const Clusters& cls2);

#endif // INTERFACE_H
