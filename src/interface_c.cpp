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
//! the mutual match until makeunique is set;
//! (reduce == (nodebase->ndsnum() < rcn.num)) || nodebase->ndsnum() == rcn.num
//!
//! \param rcn const ClusterCollection  - raw collection of clusters
//! \param makeunique=false bool  - ensure that clusters contain unique members by
//! removing the duplicates
//! \param membership=1 float  - expected membership of the nodes, >0, typically >= 1.
//! Used only for the node container preallocation to estimate the number of nodes
//! if not specified in the file header
//! \param ahash=nullptr AggHash*  - resulting hash of the loaded
//! member ids base (unique ids only are hashed, not all ids) if not nullptr
//! \param const nodebase=nullptr NodeBaseI*  - node base to filter-out or complement nodes if required
//! \param reduce=false bool  - whether to reduce collections by removing the non-matching nodes
//! or extend collections by appending those nodes them to a single "noise" cluster
//! \param lostcls=nullptr RawIds*  - indices of the lost clusters during the node base
//! synchronization
//! \param verbose=false bool  - print the number of loaded nodes to the stdout
//! \return CollectionT  - the collection is loaded successfully
Collection<Id> loadCollection(const ClusterCollection rcn, bool makeunique=false
	, float membership=1, ::AggHash* ahash=nullptr, const NodeBaseI* nodebase=nullptr
	, bool reduce=false, RawIds* lostcls=nullptr, bool verbose=false);

Collection<Id> loadCollection(const ClusterCollection rcn, bool makeunique, float membership
	, ::AggHash* ahash, const NodeBaseI* nodebase, bool reduce, RawIds* lostcls, bool verbose)
{
	assert(((reduce == (nodebase->ndsnum() < rcn.num)) || nodebase->ndsnum() == rcn.num)
		&& "Nodebase is not synced with the reduce argument");

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
			if(nodebase && reduce && !nodebase->nodeExists(did))
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
			//for(auto v: members)
			//	printf(" %u", v);
			//puts("");
			cn.m_cls.push_back(chd.release());
			// Start filling a new cluster
			chd.reset(new Cluster<Id>());
		} else if(lostcls)
			lostcls->push_back(lostcls->size() + cn.m_cls.size());
	}

	// Extend collection with a single "noise" cluster containing missed nodes if required
	if(nodebase && !reduce && cn.m_ndcs.size() < nodebase->ndsnum()) {
		// Fetch complementary nodes
		RawIds nids;
		nids.reserve(nodebase->ndsnum() - cn.m_ndcs.size());
		for(auto nid: nodebase->nodes())
			if(!cn.m_ndcs.count(nid))
				nids.push_back(nid);
		// Add complementary nodes to the
		Cluster<Id>* const  pcl = chd.get();
		pcl->members.insert(pcl->members.end(), nids.begin(), nids.end());
		for(auto nid: nids)
			cn.m_ndcs[nid].push_back(pcl);
		cn.m_cls.push_back(chd.release());
	}

	// Save some space if it is essential
	if(cn.m_cls.size() < cn.m_cls.capacity() / 2)
		cn.m_cls.shrink_to_fit();
	// Rehash the clusters and nodes for faster traversing if required
	//if(cn.m_cls.size() < cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() / 2)
	//	cn.m_cls.reserve(cn.m_cls.size());
	if(cn.m_ndcs.size() < cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() / 2)
		cn.m_ndcs.reserve(cn.m_ndcs.size());

	// Evaluate the node hash
	::AggHash  mbhash;  // Nodes hash (only unique nodes, not all the members)
	for(const auto& ndcl: cn.m_ndcs)
		mbhash.add(ndcl.first);
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

/// \brief Fetch nodes from the raw collection of clusters
///
/// \param cn const ClusterCollection  - raw collection of clusters
/// \return UniqIds  - cluster nodes
UniqIds fetchNodes(const ClusterCollection cn)
{
	UniqIds nodes;  // Uses NRVO return value optimization
	nodes.reserve(cn.num * 2);

	if(cn.nodes) {
		for(NodeId i = 0; i < cn.num; ++i) {
			const auto& ndrs = cn.nodes[i];
			if(!ndrs.ids) {
				fprintf(stderr, "WARNING %s(), the empty node ids (nominally: %u ids) is omitted\n", __FUNCTION__, ndrs.num);
				continue;
			}
			for(NodeId j = 0; j < ndrs.num; ++j)
				nodes.insert(nodes.end(), ndrs.ids[j]);
		}
	} else fprintf(stderr, "WARNING %s(), the empty input collection (nominally: %u nodes) is omitted\n", __FUNCTION__, cn.num);

	return nodes;
}

/// \brief Fetch nodebase from collection of clusters, reduced (intersection) or extended (union) one
///
/// \param cn1 ClusterCollection const  - first raw collection of clusters
/// \param cn2 ClusterCollection const  - second raw collection of clusters
/// \param reduced=false bool  - whether reduce or extend tham
/// \return NodeBase  - resulting nodebase
NodeBase fetchNodebase(const ClusterCollection cn1, const ClusterCollection cn2, bool reduced=false)
{
	NodeBase nodes;  // Uses NRVO return value optimization
	if(reduced) {
		UniqIds  nds1 = fetchNodes(cn1);
		UniqIds  nds2 = fetchNodes(cn2);
		nodes.reserve(abs(static_cast<long>(nds1.size()) - static_cast<long>(nds2.size())));
		for(auto nid: nds1)
			if(!nds2.count(nid))
				nodes.insert(nodes.end(), nid);
		for(auto nid: nds2)
			if(!nds1.count(nid))
				nodes.insert(nodes.end(), nid);
#if VALIDATE >= 2
		assert((nodes.ndsnum() <= min(nds1.size(), nds2.size())) && "Unexpected size of resulting nodes");
#endif // VALIDATE
	} else for(const auto& cn: {cn1, cn2}) {
		const auto partnds = fetchNodes(cn);
		nodes.insert(partnds.begin(), partnds.end());
	}
	return nodes;
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
	return f1x(cn1, cn2, kind, rec ? rec : &tmp, prc ? prc : &tmp, MATCH_WEIGHTED, 1, 1, 0);
}

Probability f1x(const ClusterCollection cn1, const ClusterCollection cn2, F1Kind kind
	, Probability* rec, Probability* prc, MatchKind mkind, uint8_t sync, uint8_t makeunique, uint8_t verbose)
{
#if TRACE >= 2
	if(verbose)
		printf("%s(), loading clustering collections of size: %u, %u\n", __FUNCTION__
			, cn1.num, cn2.num);
#endif // TRACE
	assert(rec && prc && "Invalid output arguments");
	// Load nodes
	const bool reduce = false;  // Whether to reduce or extend collections of clusters
	Probability res = 0;
	if(sync) {
		NodeBase ndbase = fetchNodebase(cn1, cn2, reduce);
		Collection<Id>  c1 = loadCollection(cn1, makeunique, 1, nullptr, &ndbase, reduce);
		Collection<Id>  c2 = loadCollection(cn2, makeunique, 1, nullptr, &ndbase, reduce);
		res = Collection<Id>::f1(c1, c2, static_cast<F1>(kind), *rec, *prc, static_cast<Match>(mkind), verbose);
	} else {
		Collection<Id>  c1 = loadCollection(cn1);
		Collection<Id>  c2 = loadCollection(cn2);
		res = Collection<Id>::f1(c1, c2, static_cast<F1>(kind), *rec, *prc, static_cast<Match>(mkind), verbose);
	}
	return res;
}

Probability omega(const ClusterCollection cn1, const ClusterCollection cn2)
{
	return omegax(cn1, cn2, 0, 1, 1);
}

Probability omegaExt(const ClusterCollection cn1, const ClusterCollection cn2)
{
	return omegax(cn1, cn2, 1, 1, 1);
}

Probability omegax(const ClusterCollection cn1, const ClusterCollection cn2, uint8_t ext, uint8_t sync, uint8_t makeunique)
{
	// Transform loaded and pre-processed collection to the representation
	// suitable for Omega Index evaluation
	RawClusters  cls1;
	RawClusters  cls2;
	NodeRClusters  ndrcs;

	const bool reduce = false;  // Whether to reduce or expand collections of clusters
	if(sync) {
		NodeBase ndbase = fetchNodebase(cn1, cn2, reduce);
		Collection<Id>  c1 = loadCollection(cn1, makeunique, 1, nullptr, &ndbase, reduce);
		Collection<Id>  c2 = loadCollection(cn2, makeunique, 1, nullptr, &ndbase, reduce);
		c1.template transfer<true>(cls1, ndrcs);
		c2.template transfer<false>(cls2, ndrcs);
	} else {
		Collection<Id>  c1 = loadCollection(cn1);
		Collection<Id>  c2 = loadCollection(cn2);
		c1.template transfer<true>(cls1, ndrcs);
		c2.template transfer<false>(cls2, ndrcs);
	}
	return ext ? omega<true>(ndrcs, cls1, cls2)
		: omega<false>(ndrcs, cls1, cls2);
}
