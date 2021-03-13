//! \brief Extrinsic measures evaluation interface implementation.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2021-03-11

#include <utility>  // move
#include <string>
#include <vector>
#include <unordered_set>
// For the template definitions
#include <cmath>  // sqrt
#include <algorithm>  // sort
#include <cassert>
#include "agghash.hpp"
#include "interface.hpp"
#include "interface_c.h"

using std::move;
using std::string;
using std::vector;
using std::unordered_set;

// Accessory routines ----------------------------------------------------------

// Note: a dedicated declaration id required to define default parameters
//! \brief Load collection from the provided raw collection
//! \note This is an accessory routine for C API
//! \pre All clusters in the collection are expected to be unique and not validated for
//! the mutual match until makeunique is set
//!
//! \param rcn const ClusterCollection  - raw collection of nodes
//! \param makeunique=false bool  - ensure that clusters contain unique members by
//! 	removing the duplicates
//! \param membership=1 float  - expected membership of the nodes, >0, typically >= 1.
//! Used only for the node container preallocation to estimate the number of nodes
//! if not specified in the file header
//! \param ahash=nullptr AggHash*  - resulting hash of the loaded
//! member ids base (unique ids only are hashed, not all ids) if not nullptr
//! \param const nodebase=nullptr NodeBaseI*  - node base to filter-out nodes if required
//! \param lostcls=nullptr RawIds*  - indices of the lost clusters during the node base
//! synchronization
//! \param verbose=false bool  - print the number of loaded nodes to the stdout
//! \return CollectionT  - the collection is loaded successfully
Collection<Id> loadCollection(const ClusterCollection rcn, bool makeunique=false
	, float membership=1, ::AggHash* ahash=nullptr, const NodeBaseI* nodebase=nullptr
	, RawIds* lostcls=nullptr, bool verbose=false);

Collection<Id> loadCollection(const ClusterCollection rcn, bool makeunique
	, float membership, ::AggHash* ahash, const NodeBaseI* nodebase, RawIds* lostcls, bool verbose)
{
	Collection<Id>  cn;  // Return using NRVO, named return value optimization
	if(!rcn.nodes) {
		fputs("WARNING loadCollection(), the empty input collection is omitted\n", stderr);
		return cn;
	}

	// Preallocate space for the clusters and nodes
	size_t  nsnum = rcn.num * 2;  // The (estimated) number of nodes
	if(cn.m_cls.capacity() < rcn.num)  //  * cn.m_cls.max_load_factor()
		cn.m_cls.reserve(rcn.num);
	if(cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() < nsnum)
		cn.m_ndcs.reserve(nsnum);

	// Load clusters
#if TRACE >= 2
	fprintf(stderr, "loadCollection(), expected %lu clusters, %lu nodes from %u raw node relations\n"
		, rcn.num, nsnum, rcn.num);
	if(nodebase)
		fprintf(stderr, "loadCollection(), nodebase provided with %u nodes\n", nodebase->ndsnum());
#endif // TRACE

	// Parse clusters
	::AggHash  mbhash;  // Nodes hash (only unique nodes, not all the members)
	ClusterHolder<Id>  chd(new Cluster<Id>());
	for(NodeId i = 0; i < rcn.num; ++i) {
		Cluster<Id>* const  pcl = chd.get();
		auto& members = pcl->members;
		const auto& ndrels = rcn.nodes[i];
		members.reserve(ndrels.num);
		for(NodeId j = 0; j < ndrels.num; ++j) {
			assert(ndrels.ids && "Invalid (non-allocated) node relations");
			const auto did = ndrels.ids[j];
			// Filter out nodes if required
			if(nodebase && !nodebase->nodeExists(did))
				continue;
			members.push_back(did);
			auto& ncs = cn.m_ndcs[did];
			ncs.push_back(pcl);
		}
		if(!members.empty()) {
			members.shrink_to_fit();  // Free over reserved space
			if(makeunique) {
				// Ensure or validate that members are unique
				std::sort(members.begin(), members.end());
				const auto im = unique(members.begin(), members.end());
				//const auto im = adjacent_find(members.begin(), members.end());
				if(im != members.end()) {
					fprintf(stderr, "WARNING loadCollection(), #%lu cluster contained %lu duplicated members, corrected.\n"
						, cn.m_cls.size(), distance(im, members.end()));
					// Remove associated clusters
					for(auto jm = im; jm != members.end(); ++jm)
						cn.m_ndcs[*jm].pop_back();
					// Remove the tail of duplicated node ids
					members.erase(im, members.end());
					//fprintf(stderr, "WARNING loadCollection(), #%lu cluster contains duplicated member #%lu: %u\n"
					//	, cn.m_cls.size(), distance(members.begin(), im), *im);
					//throw invalid_argument("loadCollection(), the cluster contains duplicated members\n");
				}
			}
			members.shrink_to_fit();  // Free over reserved space
			// Update hash
			for(auto v: members) {
				mbhash.add(v);
				//printf(" %u", v);
			}
			//puts("");
			cn.m_cls.push_back(chd.release());
			// Start filling a new cluster
			chd.reset(new Cluster<Id>());
		} else if(lostcls)
			lostcls->push_back(lostcls->size() + cn.m_cls.size());
	}

	// Save some space if it is essential
	if(cn.m_cls.size() < cn.m_cls.capacity() / 2)
		cn.m_cls.shrink_to_fit();
	// Rehash the clusters and nodes for faster traversing if required
	//if(cn.m_cls.size() < cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() / 2)
	//	cn.m_cls.reserve(cn.m_cls.size());
	if(cn.m_ndcs.size() < cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() / 2)
		cn.m_ndcs.reserve(cn.m_ndcs.size());
	// Assign hash to the results
	cn.m_ndshash = mbhash.hash();  // Note: required to identify the unequal node base in the processing collections
	if(ahash)
		*ahash = move(mbhash);
#if TRACE >= 2
	printf("loadCollection(), loaded %lu clusters (capacity: %lu, overhead: %0.2f %%) and"
		" %lu nodes (reserved %lu buckets, overhead: %0.2f %%) with hash %lu from %u raw node relations\n"
		, cn.m_cls.size(), cn.m_cls.capacity()
		, cn.m_cls.size() ? float(cn.m_cls.capacity() - cn.m_cls.size()) / cn.m_cls.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, cn.m_ndcs.size() ? float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndshash, rcn.num);
#elif TRACE >= 1
	if(verbose)
		printf("loadCollection(), loaded %lu clusters %lu nodes from %u raw node relations\n", cn.m_cls.size()
			, cn.m_ndcs.size(), rcn.num);
#endif

	return cn;
}

// Interface implementation ----------------------------------------------------
Probability f1p(const ClusterCollection cn1, const ClusterCollection cn2)
{
	return f1(cn1, cn2, F1_PARTPROB, nullptr, nullptr);
}

Probability f1h(const ClusterCollection cn1, const ClusterCollection cn2)
{
	return f1(cn1, cn2, F1_HARMONIC, nullptr, nullptr);
}

Probability f1(const ClusterCollection cn1, const ClusterCollection cn2, F1Kind kind
	, Probability* rec, Probability* prc)
{
	Probability  tmp;  // Temporary buffer, a placeholder
	return f1x(cn1, cn2, kind, rec ? rec : &tmp, prc ? prc : &tmp, MATCH_WEIGHTED, 0);
}

Probability f1x(const ClusterCollection cn1, const ClusterCollection cn2, F1Kind kind
	, Probability* rec, Probability* prc, MatchKind mkind, uint8_t verbose)
{
#if TRACE >= 2
	if(verbose)
		printf("%s(), loading clustering collections of size: %u, %u\n", __FUNCTION__
			, cn1.num, cn2.num);
#endif // TRACE
	// Load nodes
	const auto c1 = loadCollection(cn1);
	const auto c2 = loadCollection(cn2);
	assert(rec && prc && "Invalid output arguments");
	return Collection<Id>::f1(c1, c2, static_cast<F1>(kind), *rec, *prc, static_cast<Match>(mkind), verbose);
}

Probability omega(const ClusterCollection cn1, const ClusterCollection cn2)
{
	// Transform loaded and pre-processed collection to the representation
	// suitable for Omega Index evaluation
	RawClusters  cls1;
	RawClusters  cls2;
	NodeRClusters  ndrcs;

	// Load nodes
	auto c1 = loadCollection(cn1);
	c1.template transfer<true>(cls1, ndrcs);
	auto c2 = loadCollection(cn2);
	c2.template transfer<false>(cls2, ndrcs);
	return omega<false>(ndrcs, cls1, cls2);
}

Probability omegaExt(const ClusterCollection cn1, const ClusterCollection cn2)
{
	// Transform loaded and pre-processed collection to the representation
	// suitable for Omega Index evaluation
	RawClusters  cls1;
	RawClusters  cls2;
	NodeRClusters  ndrcs;

	// Load nodes
	auto c1 = loadCollection(cn1);
	c1.template transfer<true>(cls1, ndrcs);
	auto c2 = loadCollection(cn2);
	c2.template transfer<false>(cls2, ndrcs);
	return omega<true>(ndrcs, cls1, cls2);
}

