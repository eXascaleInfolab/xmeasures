//! \brief Extrinsic measures evaluation interface.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2021-03-11

#ifndef INTERFACE_C_H_INCLUDED
#define INTERFACE_C_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>  // uintX_t


typedef uint32_t  NodeId;  //!< Node Id type
typedef uint64_t  AccNodeId;  //!< Accumulated Node Id type
typedef float  LinkWeight;  ///< Link weight

//! \brief Node relations
typedef struct {
	NodeId  num;  //!< The number of cluster nodes
	NodeId*  ids;  //!< Node ids
	LinkWeight*  weights;  //!< Node weights in this cluster, can be NULL which means equal weights = 1
} ClusterNodes;

//! \brief Node collection (clusters)
typedef struct {
	NodeId  num;  //!< The number of node relations (clusters) in a collection
	ClusterNodes*  nodes;  //!< Relations of nodes
} ClusterCollection;

//! \brief F1 Kind
typedef enum {
	//! Not initialized
	F1_NONE = 0,
	//! Harmonic mean of the [weighted] average of the greatest (maximal) match
	//! by partial probabilities
	F1_PARTPROB,
	//! Harmonic mean of the [weighted] average of the greatest (maximal) match by F1s
	F1_HARMONIC,
	//! Arithmetic mean (average) of the [weighted] average of the greatest (maximal)
	//! match by F1s, i.e. F1-Score
	F1_AVERAGE  // Suggested by Leskovec
} F1Kind;

//! \brief Collection matching kind
typedef enum {
	MATCH_NONE = 0,  //!< Note initialized
	MATCH_WEIGHTED,  //!< Weighted matching by the number of members in each cluster (macro weighting)
	MATCH_UNWEIGHTED,  //!< Unweighted matching of each cluster (micro weighting)
	MATCH_COMBINED  //!< Combined of macro and micro weightings using geometric mean
} MatchKind;

typedef float  Probability;

//! \brief Specified F1 evaluation of the Greatest (Max) Match for the
//! multi-resolution clustering with possibly unequal node base
//!
//! Supported F1 measures are F1p <= F1h <= F1s, where:
//! - F1p  - Harmonic mean of the [weighted] average of partial probabilities,
//! 	the most discriminative and satisfies the largest number of the Formal
//! 	Constraints (homogeneity, completeness, rag bag,  size/quantity, balance);
//! - F1h  - Harmonic mean of the [weighted] average of F1s;
//! - F1a  - Average F1-Score, i.e. arithmetic mean (average) of the [weighted]
//! 	average of F1s, the least discriminative and satisfies the lowest number
//! 	of the Formal Constraints.
//!
//! of the Greatest (Max) Match [Weighted] Average Harmonic Mean evaluation
//! \note Undirected (symmetric) evaluation
//!
//! \param cn1 const ClusterCollection  - first collection of clusters (node relations)
//! \param cn2 const ClusterCollection  - second collection
//! \param kind F1Kind  - kind of F1 to be evaluated
//! \param[out] rec Probability*  - recall of cn2 relative to the ground-truth cn1 or
//! 0 if the matching strategy does not have the precision/recall notations
//! \param[out] prc Probability*  - precision of cn2 relative to the ground-truth cn1 or
//! 0 if the matching strategy does not have the precision/recall notations
//! \param mkind=MATCH_WEIGHTED MatchKind  - matching kind
//! \param sync uint8_t  - synchronize node base of the input collections, by appending
//! the lacking single-node clusters
//! \param makeunique uint8_t  - ensure that clusters contain unique members by
//! removing the duplicates
//! \param verbose=0 uint8_t  - print intermediate results to the stdout
//! \return Probability  - resulting F1_gm
Probability f1x(const ClusterCollection cn1, const ClusterCollection cn2, F1Kind kind
	, Probability* rec, Probability* prc, MatchKind mkind, uint8_t sync, uint8_t makeunique, uint8_t verbose);
Probability f1(const ClusterCollection cn1, const ClusterCollection cn2, F1Kind kind
	, Probability* rec, Probability* prc);  // MATCH_WEIGHTED, false
Probability f1p(const ClusterCollection cn1, const ClusterCollection cn2);  // MATCH_WEIGHTED, false
Probability f1h(const ClusterCollection cn1, const ClusterCollection cn2);  // MATCH_WEIGHTED, false

//! \brief (Extended) Omega Index evaluation
//!
//! \param cn1 const ClusterCollection  - first collection of clusters (node relations)
//! \param cn2 const ClusterCollection  - second collection
//! \return Probability  - omega index
Probability omega(const ClusterCollection cn1, const ClusterCollection cn2);
Probability omegaExt(const ClusterCollection cn1, const ClusterCollection cn2);

//! \brief (Extended) Omega Index evaluation
//!
//! \param cn1 const ClusterCollection  - first collection of clusters (node relations)
//! \param cn2 const ClusterCollection  - second collection
//! \param ext uint8_t  - evaluate extended omega index
//! \param sync uint8_t  - synchronize node base of the input collections, by appending the lacking single-node clusters
//! \param makeunique uint8_t  - ensure that clusters contain unique members by
//! removing the duplicates
//! \return Probability  - omega index
Probability omegax(const ClusterCollection cn1, const ClusterCollection cn2, uint8_t ext, uint8_t sync, uint8_t makeunique);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // INTERFACE_C_H_INCLUDED
