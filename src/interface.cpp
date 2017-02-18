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


using std::out_of_range;
using namespace daoc;

// SparseMatrix definitions ----------------------------------------------------
template<typename Index, typename Value>
SparseMatrix<Index, Value>::SparseMatrix(Id rows)
{
	if(rows)
		BaseT::reserve(rows);  // Preallocate hash map
}

template<typename Index, typename Value>
Value& SparseMatrix<Index, Value>::operator ()(Index i, Index j)
{
	auto& rowi = (*this)[i];
	auto ir = fast_ifind(rowi, j, bsObjOp<RowVecItem<Index, Value>>);
	if(ir == rowi.end() || ir->pos != j)
		ir = rowi.emplace(ir, j);  // Transparently Insert a new element
	return ir->val;
}

template<typename Index, typename Value>
template <typename T, enable_if_t<sizeof(T) <= sizeof(void*)>*>
Value SparseMatrix<Index, Value>::operator()(Index i, Index j) const noexcept
{
#if VALIDATE >= 2
	auto irowi = BaseT::find(i);
	if(irowi == BaseT::end())
		fprintf(stderr, "ERROR operator(), row #%u does not exist\n", i);
	auto ie = fast_ifind(*irowi, j, bsObjOp<RowVecItem<Index, Value>>)->val;
	if(irowi == BaseT::end())
		fprintf(stderr, "ERROR operator(), element #u in the row #%u does not exist\n", j, i);
	return ie->val;
#else
	return fast_ifind(*find(i), j, bsObjOp<RowVecItem<Index, Value>>)->val;
#endif // VALIDATE
}

template<typename Index, typename Value>
template <typename T, enable_if_t<(sizeof(T) > sizeof(void*)), bool>*>
const Value& SparseMatrix<Index, Value>::operator()(Index i, Index j) const noexcept
{
#if VALIDATE >= 2
	auto irowi = BaseT::find(i);
	if(irowi == BaseT::end())
		fprintf(stderr, "ERROR operator() 2, row #%u does not exist\n", i);
	auto ie = fast_ifind(*irowi, j, bsObjOp<RowVecItem<Index, Value>>)->val;
	if(irowi == BaseT::end())
		fprintf(stderr, "ERROR operator(), element #u in the row #%u does not exist\n", j, i);
	return ie->val;
#else
	return fast_ifind(*find(i), j, bsObjOp<RowVecItem<Index, Value>>)->val;
#endif // VALIDATE
}

template<typename Index, typename Value>
template <typename T, enable_if_t<sizeof(T) <= sizeof(void*)>*>
Value SparseMatrix<Index, Value>::at(Index i, Index j)
{
	auto& rowi = BaseT::at(i);
	auto ie = fast_ifind(rowi, j, bsObjOp<RowVecItem<Index, Value>>);
	if(ie == rowi.end() || ie->pos != j)
		throw out_of_range("at(), attempt to access nonexistent element #"
			+ to_string(j) + " at the row #" + to_string(i) + "\n");
	return ie->val;
}

template<typename Index, typename Value>
template <typename T, enable_if_t<(sizeof(T) > sizeof(void*)), bool>*>
const Value& SparseMatrix<Index, Value>::at(Index i, Index j)
{
	auto& rowi = BaseT::at(i);
	auto ie = fast_ifind(rowi, j, bsObjOp<RowVecItem<Index, Value>>);
	if(ie == rowi.end() || ie->pos != j)
		throw out_of_range("at() 2, element #" + to_string(j) + " in the row #"
			+ to_string(i) + " does not exist\n");
	return ie->val;
}

// Collection definitions ------------------------------------------------------
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
	printf("loadNodes() [shrinked], loaded %lu clusters (reserved %lu buckets, overhead: %0.2f %%) and"
		" %lu nodes (reserved %lu buckets, overhead: %0.2f %%) from %s\n"
		, cn.m_cls.size(), cn.m_cls.bucket_count()
		, cn.m_cls.size() ? float(cn.m_cls.bucket_count() - cn.m_cls.size()) / cn.m_cls.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, cn.m_ndcs.size() ? float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.size() * 100
			: numeric_limits<float>::infinity()
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
		AccId  csizesSum = 0;
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
	F1s  f1maxs;  // Max F1 for each cluster [of this collection, self]; Uses NRVO return value optimization
	f1maxs.reserve(m_cls.size());  // Preallocate the space

	// Traverse all clusters in the collection
	for(const auto& cl: m_cls) {
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
				// Note: only the max value for F1 is sufficient
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

Prob Collection::nmi(const Collection& cn1, const Collection& cn2, bool expbase)
{
	if(!cn1.clusters() || !cn2.clusters())
		return 0;

	auto nmi1 = cn1.nmi(cn2, expbase);
#if VALIDATE >= 2
	// Check NMI value for the inverse order of collections
	auto nmi2 = cn2.nmi(cn1, expbase);
	fprintf(stderr, "nmi(), nmi1: %G, nmi2: %G,  dnmi: %G\n", nmi1, nmi2, nmi1 - nmi2);
	assert(equal(nmi1, nmi2, (cn1.clusters() + cn2.clusters()) / 2)
		&& "nmi(), nmi is not symmetric");
#endif // VALIDATE
	return 0;
}

Prob Collection::nmi(const Collection& cn, bool expbase) const
{
#if VALIDATE >= 2
	// Note: the processing is also fine (but redundant) for the empty collections
	assert(m_cls.size() && cn.clusters() && "nmi(), non-empty collections expected");
#endif // VALIDATE
	using ClustersMatching = SparseMatrix<Cluster*, Id>;  //!< Clusters matching matrix
	using Counts = vector<Id>;  //!< Counts of the clusters
	using IndexedCounts = unordered_map<Cluster*, Id>;  //!< Indexed counts of the clusters

	ClustersMatching  clsmm = ClustersMatching(m_cls.size());  // Note: default max_load_factor is 1
	Counts  c1cts(m_cls.size(), 0);  // Counts of this collection (c1), indexed from 0
	IndexedCounts  c2icts(cn.clusters());  // Indexed counts of cn collection (c2), indexed by cls2

	// Traverse all clusters in the collection
	// Total sum of all values of the clsmm matrix, i.e. the number of
	// member nodes in both collections
	AccId  cmmsum = 0;
	Id  ic1 = 0;  // c1 index
	for(const auto& cl: m_cls) {
		// Traverse all members (node ids)
		for(auto nid: cl->members) {
			// Find Matching clusters (containing the same member node id) in the foreign collection
			const auto imcls = cn.m_ndcs.find(nid);
			// Consider the case of unequal node base, i.e. missed node
			if(imcls == cn.m_ndcs.end())
				continue;
			const auto  mclsnum = imcls->second.size();
#if VALIDATE >= 2
			assert(mclsnum && "nmo(), clusters should be present");
#endif // VALIDATE
			c1cts
#if VALIDATE >= 2
				.at(ic1)
#else
				[ic1]
#endif // VALIDATE
				+= mclsnum;
			cmmsum += mclsnum;
			for(auto mcl: imcls->second) {
				clsmm(cl.get(), mcl) += 1;
				c2icts[mcl] += 1;
			}
		}
		++ic1;
	}

	// Evaluate sum of vector counts to evaluate probabilities
	// Note: to have symmetric values normalization should be done by the max values in the row / col
#if VALIDATE >= 2
	assert(cmmsum % 2 == 0 && "nmi(), cmmsum should always be even");
#if TRACE >= 3
	fprintf(stderr, "nmi(), cls1 counts (%lu): ", c1cts.size());
#endif // TRACE
	AccId c1csum = 0;
	for(auto c1cnt: c1cts) {
		c1csum += c1cnt;
#if TRACE >= 3
		fprintf(stderr, " %u", c1cnt);
#endif // TRACE
	}

#if TRACE >= 3
	fprintf(stderr, "\nnmi(), cls2 counts (%lu): ", c2icts.size());
#endif // TRACE
	AccId c2csum = 0;
	for(auto& c2ict: c2icts) {
		c2csum += c2ict.second;
#if TRACE >= 3
		fprintf(stderr, " %u", c2ict.second);
#endif // TRACE
	}
#if TRACE >= 3
	fputs("\n", stderr);
#endif // TRACE

	if(c1csum != cmmsum || c2csum != cmmsum) {
		fprintf(stderr, "nmi(), c1csum: %lu, c2csum: %lu, cmmsum: %lu\n", c1csum, c2csum, cmmsum);
		assert(0 && "nmi(), rows accumulation is invalid");
	}
#endif // VALIDATE

	// Evaluate probabilities
#if TRACE >= 2
	fprintf(stderr, "nmi(), nmi is evaluating with base %c, %lu total member nodes\n"
		, expbase ? 'e' : '2', cmmsum);
#endif // TRACE
	AccProb (*const clog)(AccProb) = expbase ? log : log2;  // log to be used
#if VALIDATE >= 2
	AccProb  psum = 0;  // Sum of probabilities over the whole matrix
#endif // VALIDATE
	AccProb  mi = 0;  // Accumulated mutual probability over the matrix, I(cn1, cn2) in exp base
	AccProb  h1 = 0;  // H(cn1) - information size of the cn1 in exp base
	ic1 = 0;  // c1 index
	// Traverse matrix of the mutual information evaluate total mutual probability and
	// mutual probabilities for each collection
	for(auto& icm: clsmm) {
		// Travers row
			auto  c1cnorm = c1cts  // Accumulated value of the current cluster from cn1
#if VALIDATE >= 2
				.at(ic1)
#else
				[ic1]
#endif // VALIDATE
			;
		// Consider that the cluster may not have any matches at all, with can be
		// possible only if the node base is not synchronized
		if(!c1cnorm) {
#if VALIDATE >= 2
			assert(nodes() != cn.nodes() && "nmi(), node bases are expected to be distinct");
#endif // VALIDATE
			continue;
		}

		for(auto& icmr: icm.second) {
			if(!icmr.val)
				continue;
			// Evaluate mutual probability of the cluster (divide by multiplication of counts of both clusters)
			AccProb  mcprob = AccProb(icmr.val) / (c1cnorm * AccId(c2icts.at(icmr.pos))) * cmmsum;
#if VALIDATE >= 2
			psum += mcprob;
			assert(mcprob > 0 && mcprob <= 1 && c2icts.at(icmr.pos)
				&& "nmi(), invalid probability or multiplier");
#endif // VALIDATE
			// Accumulate total normalized mutual information
			// Note: e base is used instead of 2 to have absolute entropy instead of bits length
			mi -= mcprob * clog(mcprob);
		}

		// Evaluate information size of the ic1 cluster in the cn1
		AccProb  cprob = AccProb(c1cnorm) / cmmsum;
#if VALIDATE >= 2
		assert(cprob > 0 && cprob <= 1 && "nmi(), invalid probability");
#endif // VALIDATE
		h1 -= cprob * clog(cprob);

		// Update c1cts index
		++ic1;
	}
#if VALIDATE >= 2
	fprintf(stderr, "nmi(), psum: %G\n", psum);
#endif // VALIDATE

	// Evaluate information size cn2 clusters
	AccProb  h2 = 0;  // H(cn2) - information size of the cn1 in exp base
	for(auto& c2ict: c2icts) {
		if(!c2ict.second)
			continue;
		AccProb  cprob = AccProb(c2ict.second) / cmmsum;
#if VALIDATE >= 2
		assert(cprob > 0 && cprob <= 1 && "nmi() 2, invalid probability");
#endif // VALIDATE
		h2 -= cprob * clog(cprob);
	}

	Prob  nmi = mi / (h1 + h2);  //(h1 >= h2 ? h1 : h2);
#if TRACE >= 2
	fprintf(stderr, "nmi(),  mi: %G,  h1: %G, h2: %G,  nmi: %G\n", mi, h1, h2, nmi);
#endif // TRACE

	return nmi;
}
