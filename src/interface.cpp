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

Collection Collection::load(const char* filename, float membership)
{
	Collection  cn;  // Return using NRVO, named return value optimization

	// Open file
	NamedFileWrapper  file(filename, "r");
	if(!file) {
		perror(string("ERROR load(), failed on opening ").append(filename).c_str());
		return cn;
	}

	constexpr size_t  FILESIZE_INVALID = size_t(-1);
	const size_t fsize = file.size();
	if(!fsize) {
		fputs(("WARNING load(), the file '" + file.name()
			+ " is empty, skipped\n").c_str(), stderr);
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
		if(!ndsnum && fsize != FILESIZE_INVALID) {  // File length fetching failed
			ndsnum = estimateCnlNodes(fsize, membership);
#if TRACE >= 2
			fprintf(stderr, "load(), %lu estimated nodes from %lu bytes\n", ndsnum, fsize);
#endif // TRACE
		}
		clsnum = estimateClusters(ndsnum, membership);
	} else {
		if(!ndsnum)
			ndsnum = clsnum * clsnum / membership;  // The expected number of nodes
	}
#if TRACE >= 2
	fprintf(stderr, "load(), expected %lu clusters, %lu nodes from %lu input bytes\n"
		, clsnum, ndsnum, fsize);
#endif // TRACE

	// Preallocate space for the clusters and nodes
	if(cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() < clsnum)
		cn.m_cls.reserve(clsnum);
	if(cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() < ndsnum)
		cn.m_ndcs.reserve(ndsnum);

	// Parse clusters
	// ATTENTION: without '\n' delimiter the terminating '\n' is read as an item
	constexpr char  mbdelim[] = " \t\n";  // Delimiter for the members
	// Estimate the number of chars per node, floating number
	const float  ndchars = ndsnum ? (fsize != FILESIZE_INVALID
		? fsize / float(ndsnum) : log10(float(ndsnum)) + 1)  // Note: + 1 to consider the leading space
		: 1.f;
#if VALIDATE >= 2
	//fprintf(stderr, "load(), ndchars: %.4G\n", ndchars);
	assert(ndchars >= 1 && "load(), ndchars invalid");
#endif // VALIDATE
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
				fprintf(stderr, "WARNING load(), empty cluster"
					" exists: '%s', skipped\n", cidstr);
				continue;
			}
		}

		// Parse remained node ids and load cluster members
		auto icl = cn.m_cls.emplace_hint(cn.m_cls.end(), new Cluster());
		Cluster* const  pcl = icl->get();
		auto& members = pcl->members;
		members.reserve(line.length() / ndchars);  // Note: strtok() does not affect line.length()
		do {
			// Note: only node id is parsed, share part is skipped if exists,
			// but potentially can be considered in NMI and F1 evaluation.
			// In the latter case abs diff of shares instead of co occurrence
			// counting should be performed.
			Id  nid = strtoul(tok, nullptr, 10);
#if VALIDATE >= 2
			if(!nid && tok[0] != '0') {
				fprintf(stderr, "WARNING load(), conversion error of '%s' into 0: %s\n"
					, tok, strerror(errno));
				continue;
			}
#endif // VALIDATE
			members.push_back(nid);
			cn.m_ndcs[nid].push_back(pcl);
		} while((tok = strtok(nullptr, mbdelim)));
		members.shrink_to_fit();  // Free over reserved space
	} while(line.readline(file));
	// Rehash the clusters and nodes for faster traversing if required
	if(cn.m_cls.size() < cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() / 2)
		cn.m_cls.reserve(cn.m_cls.size());
	if(cn.m_ndcs.size() < cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() / 2)
		cn.m_ndcs.reserve(cn.m_ndcs.size());
#if TRACE >= 2
	printf("loadNodes() [shrinked], loaded %lu clusters (reserved %lu buckets, overhead: %0.4f %%) and"
		" %lu nodes (reserved %lu buckets, overhead: %0.4f %%) from %s\n"
		, cn.m_cls.size(), cn.m_cls.bucket_count()
		, float(cn.m_cls.bucket_count() - cn.m_cls.size()) / cn.m_cls.bucket_count() * 100
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.bucket_count() * 100
		, file.name().c_str());
#elif TRACE >= 1
	printf("Loaded %lu clusters %lu nodes from %s\n", cn.m_cls.size()
		, cn.m_ndcs.size(), file.name().c_str());
#endif

	return cn;
}

Prob Collection::f1mah(const Collection& cn1, const Collection& cn2, bool weighted)
{
#if TRACE >= 3
	fputs("f1mah(), F1 Max Avg of the first collection\n", stderr);
#endif // TRACE
	const AccProb  f1ma1 = cn1.f1MaxAvg(cn2, weighted);
#if TRACE >= 3
	fputs("f1mah(), F1 Max Avg of the second collection\n", stderr);
#endif // TRACE
	const AccProb  f1ma2 = cn2.f1MaxAvg(cn1, weighted);
#if TRACE >= 2
	fprintf(stderr, "f1mah(),  f1ma1: %.3G,  f1ma2: %.3G\n", f1ma1, f1ma2);
#endif // TRACE
	return 2 * f1ma1 / (f1ma1 + f1ma2) * f1ma2;
}

AccProb Collection::f1MaxAvg(const Collection& cn, bool weighted) const
{
	AccProb  aggf1max = 0;
	const auto  f1maxs = clsF1Max(cn);
	if(weighted) {
		const Id  clsnum = m_cls.size();
#if VALIDATE >= 2
		assert(f1maxs.size() == clsnum
			&& "f1MaxAvg(), F1s are not synchronized with the clusters");
#endif // VALIDATE
		uint64_t  csizesSum = 0;
		auto icl = m_cls.begin();
		for(size_t i = 0; i < clsnum; ++i) {
			auto csize = (*icl++)->members.size();
			aggf1max += f1maxs[i] * csize;
			csizesSum += csize;
		}
#if VALIDATE >= 2
		assert(csizesSum >= clsnum && "f1MaxAvg(), invalid sum of the cluster sizes");
#endif // VALIDATE
		aggf1max /= csizesSum / AccProb(clsnum);
	} else for(auto f1max: f1maxs)
		aggf1max += f1max;
#if VALIDATE >= 1
	if(aggf1max >= f1maxs.size() + 1)
		throw std::overflow_error("f1MaxAvg(), aggf1max is invalid (larger than"
			" the number of clusters)\n");
#endif // VALIDATE
	return aggf1max / f1maxs.size();
}

F1s Collection::clsF1Max(const Collection& cn) const
{
//	// Note: crels defined outside the cycle to avoid reallocations
//	ClusterPtrs  crels;  // Cluster relations from the evaluating one to the foreign collection clusters
//	crels.reserve(sqrt(cn.m_cls.size()));  // Preallocate the space

	F1s  f1maxs;  // Max F1 for each cluster [of this collection, self]; Uses NRVO return value optimization
	f1maxs.reserve(m_cls.size());  // Preallocate the space

	// Traverse all clusters in the collection
	for(const auto& cl: m_cls) {
		//const Id  csize = cl->size();
		Prob  f1max = 0;
		// Traverse all members (node ids)
		for(auto nid: cl->members) {
			// Find Matching clusters (containing the same member node id) in the foreign collection
			const auto imcls = cn.m_ndcs.find(nid);
			// Consider the case of unequal node base, i.e. missed node
			if(imcls == cn.m_ndcs.end())
				continue;
			for(auto mcl: imcls->second) {
				mcl->counter(cl.get());
				// Note: need targ clusters for NMI, but only the max value for F1
				//accBest(cands, f1max, *imc, relF1(mcl->counter(), csize, mcl->size()), csize)
				const Prob  cf1 = cl->f1(mcl->counter(), mcl->members.size());
				if(f1max < cf1)  // Note: <  usage is fine here
					f1max = cf1;
			}
		}
		f1maxs.push_back(f1max);
#if TRACE >= 3
		fprintf(stderr, "  %p (%lu): %.3G", cl.get(), cl->members.size(), f1max);
#endif // TRACE
	}
#if TRACE >= 3
	fputs("\n", stderr);
#endif // TRACE
	return f1maxs;
}

Prob evalNmi(const Collection& cn1, const Collection& cn2)
{
	return 0;
}
