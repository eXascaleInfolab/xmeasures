//! \brief Extrinsic measures evaluation interface.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include <cstring>  // strlen, strtok
//#include <cmath>  // sqrt
#include "operations.hpp"
#include "interface.h"


using namespace daoc;

//Collection loadCollection(const char* filename, float membership)
//{
//	Collection  cn;  // Return using NRVO, named return value optimization
//
//	// Open file
//	NamedFileWrapper  file(filename, "r");
//	if(!file) {
//		perror(string("ERROR loadClustering(), failed on opening ").append(filename).c_str());
//		return cn;
//	}
//
//	// Load clusters
//	// Note: CNL [CSN] format only is supported
//	size_t  clsnum = 0;  // The number of clusters
//	size_t  ndsnum = 0;  // The number of nodes
//	// Note: strings defined out of the cycle to avoid reallocations
//	StringBuffer  line;  // Reading line
//	// Parse header and read the number of clusters if specified
//	parseCnlHeader(file, line, clsnum, ndsnum);
//
//	// Estimate the number of clusters in the file if not specified
//	if(!clsnum) {
//		size_t  cmsbytes = -1;  // Bytes in communities file
//		if(!ndsnum) {
//			cmsbytes = file.size();
//			if(cmsbytes != size_t(-1))  // File length fetching failed
//				ndsnum = estimateNodes(cmsbytes, membership);
//		}
//		clsnum = estimateClusters(ndsnum, membership);
//#if TRACE >= 2
//		fprintf(stderr, "loadClustering(), %lu nodes (estimated: %u)"
//			", %lu estimated clusters\n", ndsnum, cmsbytes != size_t(-1), clsnum);
//#endif // TRACE
//	} else {
//#if TRACE >= 2
//		fprintf(stderr, "loadClustering(), specified %lu clusters, %lu nodes\n"
//			, clsnum, ndsnum);
//#endif // TRACE
//		if(!ndsnum)
//			ndsnum = clsnum * clsnum / membership;  // The expected number of nodes
//	}
//
//	// Preallocate space for the clusters and nodes
//	if(cn.clusters.bucket_count() * cn.clusters.max_load_factor() < clsnum)
//		cn.clusters.reserve(clsnum);
//	if(cn.nodecls.bucket_count() * cn.nodecls.max_load_factor() < ndsnum)
//		cn.nodecls.reserve(ndsnum);
//
//	// Parse clusters
//	// ATTENTION: without '\n' delimiter the terminating '\n' is read as an item
//	constexpr char  mbdelim[] = " \t\n";  // Delimiter for the members
//	do {
//		// Skip cluster id if specified and parse first node id
//		char *tok = strtok(line, mbdelim);  // const_cast<char*>(line.data())
//
//		// Skip comments
//		if(!tok || tok[0] == '#')
//			continue;
//		// Skip the cluster id if present
//		if(tok[strlen(tok) - 1] == '>') {
//			const char* cidstr = tok;
//			tok = strtok(nullptr, mbdelim);
//			// Skip empty clusters, which actually should not exist
//			if(!tok) {
//				fprintf(stderr, "WARNING loadClustering(), empty cluster"
//					" exists: '%s', skipped\n", cidstr);
//				continue;
//			}
//		}
//
//		// Parse remained node ids and load cluster members
//		auto icl = cn.clusters.emplace_hint(cn.clusters.end(), new Cluster());
//		Cluster* const  pcl = icl->get();
//		auto& members = pcl->members;
//		do {
//			// Note: only node id is parsed, share part is skipped if exists,
//			// but potentially can be considered in NMI and F1 evaluation.
//			// In the latter case abs diff of shares instead of co occurrence
//			// counting should be performed.
//			Id  nid = strtoul(tok, nullptr, 10);
//#if HEAVY_VALIDATION >= 2
//			if(!nid && tok[0] != '0') {
//				fprintf(stderr, "WARNING loadClustering(), conversion error of '%s' into 0: %s\n"
//					, tok, strerror(errno));
//				continue;
//			}
//#endif // HEAVY_VALIDATION
//			members.push_back(nid);
//			cn.nodecls[nid].push_back(pcl);
//		} while((tok = strtok(nullptr, mbdelim)));
//	} while(line.readline(file));
//	// Rehash the clusters and nodes for faster traversing if required
//	if(cn.clusters.size() < cn.clusters.bucket_count() * cn.clusters.max_load_factor() / 2)
//		cn.clusters.reserve(cn.clusters.size());
//	if(cn.nodecls.size() < cn.nodecls.bucket_count() * cn.nodecls.max_load_factor() / 2)
//		cn.nodecls.reserve(cn.nodecls.size());
//#if TRACE >= 2
//	printf("loadNodes(), loaded %lu clusters (reserved %lu buckets, overhead: %0.4f %%) and"
//		" %lu nodes (reserved %lu buckets, overhead: %0.4f %%) from %s\n"
//		, cn.clusters.size(), cn.clusters.bucket_count()
//		, float(cn.clusters.bucket_count() - cn.clusters.size()) / cn.clusters.bucket_count() * 100
//		, cn.nodecls.size(), cn.nodecls.bucket_count()
//		, float(cn.nodecls.bucket_count() - cn.nodecls.size()) / cn.nodecls.bucket_count() * 100
//		, file.name().c_str());
//#else
//	printf("Loaded %lu clusters %lu nodes from %s\n", cn.clusters.size()
//		, cn.nodecls.size(), file.name().c_str());
//#endif
//
//	return cn;
//}

F1s Collection::mbsF1Max(const Collection& cn) const
{
//	// Note: crels defined outside the cycle to avoid reallocations
//	ClusterPtrs  crels;  // Cluster relations from the evaluating one to the foreign collection clusters
//	crels.reserve(sqrt(cn.m_cls.size()));  // Preallocate the space

	F1s  f1maxs;  // Max F1 for each cluster [of this collection, self]
	f1maxs.reserve(cn.m_cls.size());  // Preallocate the space

	// Traverse all clusters in the collection
	for(const auto& cl: m_cls) {
		//const Id  csize = cl->size();
		Prob  rf1max = 0;
		// Traverse all members (node ids)
		for(auto nid: cl->members) {
			// Find Matching clusters (containing the same member node id) in the foreign collection
			const auto& mcls = cn.m_ndcs.at(nid);
			const auto imc = fast_ifind(mcls.begin(), mcls.end(), cl.get(), bsVal<Cluster*>);
			if(imc != mcls.end() && *imc == cl.get())
				(*imc)->counter(cl.get());
				// Note: need targ clusters for NMI, but only the max value for F1
				//accBest(cands, rf1max, *imc, relF1((*imc)->counter(), csize, (*imc)->size()), csize)
				const Prob  crf1 = cl->rf1((*imc)->counter(), (*imc)->members.size());
				if(rf1max < crf1)  // Note: <  usage is fine here
					rf1max = crf1;
		}
		f1maxs.push_back(rf1max);
	}
	return f1maxs;
}

AccProb Collection::f1MaxAvg(const Collection& cn) const
{
	AccProb  aggf1max = 0;
	const auto  f1maxs = mbsF1Max(cn);
	for(auto f1max: mbsF1Max(cn))
		aggf1max += f1max;
	return aggf1max / f1maxs.size();
}

Prob Collection::f1mah(const Collection& cn1, const Collection& cn2)
{
	const AccProb  f1ma1 = cn1.f1MaxAvg(cn2);
	const AccProb  f1ma2 = cn2.f1MaxAvg(cn1);
	return 2 * f1ma1 / (f1ma1 + f1ma2) * f1ma2;
}

Collection Collection::load(const char* filename, float membership)
{
	Collection  cn;  // Return using NRVO, named return value optimization

	// Open file
	NamedFileWrapper  file(filename, "r");
	if(!file) {
		perror(string("ERROR loadClustering(), failed on opening ").append(filename).c_str());
		return cn;
	}

	// Load clusters
	// Note: CNL [CSN] format only is supported
	size_t  clsnum = 0;  // The number of clusters
	size_t  ndsnum = 0;  // The number of nodes
	// Note: strings defined out of the cycle to avoid reallocations
	StringBuffer  line;  // Reading line
	// Parse header and read the number of clusters if specified
	parseCnlHeader(file, line, clsnum, ndsnum);

	// Estimate the number of clusters in the file if not specified
	if(!clsnum) {
		size_t  cmsbytes = -1;  // Bytes in communities file
		if(!ndsnum) {
			cmsbytes = file.size();
			if(cmsbytes != size_t(-1))  // File length fetching failed
				ndsnum = estimateNodes(cmsbytes, membership);
		}
		clsnum = estimateClusters(ndsnum, membership);
#if TRACE >= 2
		fprintf(stderr, "loadClustering(), %lu nodes (estimated: %u)"
			", %lu estimated clusters\n", ndsnum, cmsbytes != size_t(-1), clsnum);
#endif // TRACE
	} else {
#if TRACE >= 2
		fprintf(stderr, "loadClustering(), specified %lu clusters, %lu nodes\n"
			, clsnum, ndsnum);
#endif // TRACE
		if(!ndsnum)
			ndsnum = clsnum * clsnum / membership;  // The expected number of nodes
	}

	// Preallocate space for the clusters and nodes
	if(cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() < clsnum)
		cn.m_cls.reserve(clsnum);
	if(cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() < ndsnum)
		cn.m_ndcs.reserve(ndsnum);

	// Parse clusters
	// ATTENTION: without '\n' delimiter the terminating '\n' is read as an item
	constexpr char  mbdelim[] = " \t\n";  // Delimiter for the members
	do {
		// Skip cluster id if specified and parse first node id
		char *tok = strtok(line, mbdelim);  // const_cast<char*>(line.data())

		// Skip comments
		if(!tok || tok[0] == '#')
			continue;
		// Skip the cluster id if present
		if(tok[strlen(tok) - 1] == '>') {
			const char* cidstr = tok;
			tok = strtok(nullptr, mbdelim);
			// Skip empty clusters, which actually should not exist
			if(!tok) {
				fprintf(stderr, "WARNING loadClustering(), empty cluster"
					" exists: '%s', skipped\n", cidstr);
				continue;
			}
		}

		// Parse remained node ids and load cluster members
		auto icl = cn.m_cls.emplace_hint(cn.m_cls.end(), new Cluster());
		Cluster* const  pcl = icl->get();
		auto& members = pcl->members;
		do {
			// Note: only node id is parsed, share part is skipped if exists,
			// but potentially can be considered in NMI and F1 evaluation.
			// In the latter case abs diff of shares instead of co occurrence
			// counting should be performed.
			Id  nid = strtoul(tok, nullptr, 10);
#if HEAVY_VALIDATION >= 2
			if(!nid && tok[0] != '0') {
				fprintf(stderr, "WARNING loadClustering(), conversion error of '%s' into 0: %s\n"
					, tok, strerror(errno));
				continue;
			}
#endif // HEAVY_VALIDATION
			members.push_back(nid);
			cn.m_ndcs[nid].push_back(pcl);
		} while((tok = strtok(nullptr, mbdelim)));
	} while(line.readline(file));
	// Rehash the clusters and nodes for faster traversing if required
	if(cn.m_cls.size() < cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() / 2)
		cn.m_cls.reserve(cn.m_cls.size());
	if(cn.m_ndcs.size() < cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() / 2)
		cn.m_ndcs.reserve(cn.m_ndcs.size());
#if TRACE >= 2
	printf("loadNodes(), loaded %lu clusters (reserved %lu buckets, overhead: %0.4f %%) and"
		" %lu nodes (reserved %lu buckets, overhead: %0.4f %%) from %s\n"
		, cn.m_cls.size(), cn.m_cls.bucket_count()
		, float(cn.m_cls.bucket_count() - cn.m_cls.size()) / cn.m_cls.bucket_count() * 100
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.bucket_count() * 100
		, file.name().c_str());
#else
	printf("Loaded %lu clusters %lu nodes from %s\n", cn.m_cls.size()
		, cn.m_ndcs.size(), file.name().c_str());
#endif

	return cn;
}

//Prob evalF1(const Collection& cn1, const Collection& cn2)
//{
//    return 0;
//}

Prob evalNmi(const Collection& cn1, const Collection& cn2)
{
	return 0;
}



// Main types definitions ------------------------------------------------------
////! \brief Cluster contains of the unique nodes
//class Cluster {
//public:
//	using Members = set<Id>;
//	using IMembers = Members::iterator;
//
//private:
//	Members  m_members;  //!< Members (ids of the nodes)
//	Size  m_sum;  //!< Sum of the members
//	Size  m_sum2;  //!< Sum of the squared members
//
//protected:
//    //! \brief Update cluster statistics
//    //!
//    //! \param node Id
//    //! \return void
//	void updateStat(Id node) noexcept
//	{
//		m_sum += node;
//		m_sum2 += node * node;
//	}
//
//public:
//    //! \brief Validate the cluster
//    //! Throw an exception in case of overflow or other issues
//    //!
//    //! \return void
//	inline void validate();
//
//    //! \brief Add node to the cluster if the cluster hasn't had it
//    //!
//    //! \param node Id  - node id to be added
//    //! \return bool  - whether the node is added
//	inline bool extend(Id node);
////    //! \return pair<IMembers, bool> extend(Id node)  - iterator to the target node
////    //! 	and whether insertion was made (or the node already was present)
////	inline pair<IMembers, bool> extend(Id node);
//
////    //! \brief Add node to the cluster if the cluster hasn't had it
////    //!
////    //! \param node  - node id to be added
////    //! \param ims  - insertion hint
////    //! \return pair<IMembers, bool> extend(Id node)  - iterator to the target node
////    //! 	and whether insertion was made (or the node already was present)
////	inline pair<IMembers, bool> extend(Id node, IMembers ims);
//
//    //! \brief Operator less
//    //!
//    //! \param cl const Cluster&  - candidate cluster to be compared
//    //! \return bool  - whether this cluster less than cl
//	inline bool operator<(const Cluster& cl) const noexcept;
//};

// Will be used in the xmeasure
////! Clusters type, ordered by the cluster size
//using Clusters = set<Cluster>;

// Main types definitions ------------------------------------------------------
//void Cluster::validate()
//{
//	if(m_sum2 < m_sum)
//		throw overflow_error("validate(), m_sum2 is overflowed\n");
//}
//
//bool Cluster::extend(Id node)
//{
//	bool added = m_members.insert(node).second;
//	if(added)
//		updateStat(node);
//	return added;
//}
//
////pair<Cluster::IMembers, bool> Cluster::extend(Id node)
////{
////	auto res = m_members.insert(node);  // Uses NRVO return value optimization
////	if(res.second)
////		updateStat(node);
////	return res;
////}
////
////pair<Cluster::IMembers, bool> Cluster::extend(Id node, IMembers ims)
////{
////	auto res = m_members.insert(ims, node);  // Uses NRVO return value optimization
////	if(res.second)
////		updateStat(node);
////	return res;
////}
//
//bool Cluster::operator<(const Cluster& cl) const noexcept
//{
//	return m_members.size() < cl.m_members.size() || (m_members.size() == cl.m_members.size()
//		&& (m_sum < cl.m_sum || (m_sum == cl.m_sum && m_sum2 < cl.m_sum2)));
//}
