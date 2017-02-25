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
#include <bitset>
//#include <cmath>  // sqrt
#include "operations.hpp"
#include "interface.h"


using std::out_of_range;
using std::bitset;
using namespace daoc;

// SparseMatrix definitions ----------------------------------------------------
template <typename Index, typename Value>
SparseMatrix<Index, Value>::SparseMatrix(Id rows)
{
	if(rows)
		BaseT::reserve(rows);  // Preallocate hash map
}

template <typename Index, typename Value>
Value& SparseMatrix<Index, Value>::operator ()(Index i, Index j)
{
	auto& rowi = (*this)[i];
	auto ir = fast_ifind(rowi, j, bsObjOp<RowVecItem<Index, Value>>);
	if(ir == rowi.end() || ir->pos != j)
		ir = rowi.emplace(ir, j);  // Transparently Insert a new element
	return ir->val;
}

template <typename Index, typename Value>
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

template <typename Index, typename Value>
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

template <typename Index, typename Value>
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

template <typename Index, typename Value>
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

// Cluster definition ----------------------------------------------------------
Cluster::Cluster(): members(), counter()
{
#if VALIDATE >= 1
	assert(!mbscont && "Cluster(), contin should be 0");  // Note: ! is fine here
#endif // VALIDATE
}

string to_string(Evaluation eval, bool bitstr)
{
	static_assert(sizeof(Evaluation) == sizeof(EvalBase)
		, "to_string(), Evaluation type must be the same size as EvalBase");
	// Convert to bit string
	if(bitstr)
		return bitset<sizeof(Evaluation) * 8>(static_cast<EvalBase>(eval))
			.to_string().insert(0, "0b");

	// Convert to semantic string
	string  val;
	switch(eval) {
	case Evaluation::MULTIRES:
		val = "MULTIRES";
		break;
	case Evaluation::OVERLAPPING:
		val = "OVERLAPPING";
		break;
	case Evaluation::MULRES_OVP:
		val = "MULRES_OVP";
		break;
	case Evaluation::NONE:
	default:
		val = "NONE";
	}
	return val;
}

// Collection definitions ------------------------------------------------------
Collection Collection::load(const char* filename, ::AggHash* ahash, float membership)
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
	size_t  csnum = 0;  // The number of clusters
	size_t  nsnum = 0;  // The number of nodes
	// Note: strings defined out of the cycle to avoid reallocations
	StringBuffer  line;  // Reading line
	// Parse header and read the number of clusters if specified
	parseCnlHeader(file, line, csnum, nsnum);

	// Estimate the number of nodes in the file if not specified
	if(!nsnum) {
		if(fsize != FILESIZE_INVALID) {  // File length fetching failed
			nsnum = estimateCnlNodes(fsize, membership);
#if TRACE >= 2
			fprintf(stderr, "load(), %lu estimated nodes from %lu bytes\n", nsnum, fsize);
#endif // TRACE
		} else if(csnum)
			nsnum = 2 * csnum; // / membership;  // Note: use optimistic estimate instead of pessimistic (square / membership) to not overuse the memory
	}
	// Estimate the number of clusters in the file if not specified
	if(!csnum && nsnum)
		csnum = estimateClusters(nsnum, membership);
#if TRACE >= 2
	fprintf(stderr, "load(), expected %lu clusters, %lu nodes from %lu input bytes\n"
		, csnum, nsnum, fsize);
#endif // TRACE

	// Preallocate space for the clusters and nodes
	if(cn.m_cls.bucket_count() * cn.m_cls.max_load_factor() < csnum)
		cn.m_cls.reserve(csnum);
	if(cn.m_ndcs.bucket_count() * cn.m_ndcs.max_load_factor() < nsnum)
		cn.m_ndcs.reserve(nsnum);

	// Parse clusters
	// ATTENTION: without '\n' delimiter the terminating '\n' is read as an item
	constexpr char  mbdelim[] = " \t\n";  // Delimiter for the members
	// Estimate the number of chars per node, floating number
	const float  ndchars = nsnum ? (fsize != FILESIZE_INVALID
		? fsize / float(nsnum) : log10(float(nsnum) + 1))  // Note: + 1 to consider the leading space
		: 1.f;
#if VALIDATE >= 2
	//fprintf(stderr, "load(), ndchars: %.4G\n", ndchars);
	assert(ndchars >= 1 && "load(), ndchars invalid");
#endif // VALIDATE
	const bool  evalhash = ahash;
	::AggHash  mbhash;  // Members hash
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
		auto icl = cn.m_cls.emplace(new Cluster()).first;
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
			auto& ncs = cn.m_ndcs[nid];
			// Update hash if required
			if(evalhash && ncs.empty())
				mbhash.add(nid);
			ncs.push_back(pcl);
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
		" %lu nodes (reserved %lu buckets, overhead: %0.2f %%) with hash %lu (%s) from %s\n"
		, cn.m_cls.size(), cn.m_cls.bucket_count()
		, cn.m_cls.size() ? float(cn.m_cls.bucket_count() - cn.m_cls.size()) / cn.m_cls.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, cn.m_ndcs.size() ? float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.size() * 100
			: numeric_limits<float>::infinity()
		, mbhash.hash(), evalhash ? "evaluated" : "omitted", file.name().c_str());
#elif TRACE >= 1
	printf("Loaded %lu clusters %lu nodes from %s\n", cn.m_cls.size()
		, cn.m_ndcs.size(), file.name().c_str());
#endif
	// Assign hash to the results
	if(evalhash) {
#if VALIDATE >= 2
		assert(ahash && "loadNodes(), ahash should be valid");
#endif // VALIDATE
		*ahash = move(mbhash);
	}

	return cn;
}

Prob Collection::f1gm(const Collection& cn1, const Collection& cn2, bool weighted
	, bool prob)
{
#if TRACE >= 3
	fputs("f1gm(), F1 Max Avg of the first collection\n", stderr);
#endif // TRACE
	const AccProb  f1ga1 = cn1.avggms(cn2, weighted, prob);
#if TRACE >= 3
	fputs("f1gm(), F1 Max Avg of the second collection\n", stderr);
#endif // TRACE
	const AccProb  f1ga2 = cn2.avggms(cn1, weighted, prob);
#if TRACE >= 2
	fprintf(stderr, "f1gm(),  f1ga1: %.3G, f1ga2: %.3G\n", f1ga1, f1ga2);
#endif // TRACE
	return 2 * f1ga1 / (f1ga1 + f1ga2) * f1ga2;
}

AccProb Collection::avggms(const Collection& cn, bool weighted, bool prob) const
{
	AccProb  accgm = 0;
	const auto  gmats = gmatches(cn, prob);
	if(weighted) {
		const Id  csnum = m_cls.size();
#if VALIDATE >= 2
		assert(gmats.size() == csnum
			&& "avggms(), matches are not synchronized with the clusters");
#endif // VALIDATE
		AccId  csizesSum = 0;
		auto icl = m_cls.begin();
		for(size_t i = 0; i < csnum; ++i) {
			auto csize = (*icl++)->members.size();
			accgm += gmats[i] * csize;
			csizesSum += csize;
		}
#if VALIDATE >= 2
		assert(csizesSum >= csnum && "avggms(), invalid sum of the cluster sizes");
#endif // VALIDATE
		accgm /= csizesSum / AccProb(csnum);
	} else for(auto gm: gmats)
		accgm += gm;
#if VALIDATE >= 1
	if(accgm >= gmats.size() + 1)
		throw std::overflow_error("avggms(), accgm is invalid (larger than"
			" the number of clusters)\n");
#endif // VALIDATE
	return accgm / gmats.size();
}

Probs Collection::gmatches(const Collection& cn, bool prob) const
{
	// Greatest matches (Max F1 or partial probability) for each cluster [of this collection, self];
	Probs  gmats;  // Uses NRVO return value optimization
	gmats.reserve(m_cls.size());  // Preallocate the space

	auto fmatch = prob ? &Cluster::pprob : &Cluster::f1;  // Function evaluating value of the match

	// Traverse all clusters in the collection
	for(const auto& cl: m_cls) {
		Prob  gmatch = 0; // Greatest value of the match (F1 or partial probability)
		// Traverse all members (node ids)
		for(auto nid: cl->members) {
			// Find Matching clusters (containing the same member node id) in the foreign collection
			const auto imcls = cn.m_ndcs.find(nid);
			// Consider the case of unequal node base, i.e. missed node
			if(imcls == cn.m_ndcs.end())
				continue;
			for(auto mcl: imcls->second) {
				mcl->counter(cl.get());
				// Note: only the max value for match is sufficient
				const Prob  match = ((*cl).*fmatch)(mcl->counter(), mcl->members.size());
				if(gmatch < match)  // Note: <  usage is fine here
					gmatch = match;
			}
		}
		gmats.push_back(gmatch);
#if TRACE >= 3
		fprintf(stderr, "  %p (%lu): %.3G", cl.get(), cl->members.size(), gmatch);
#endif // TRACE
	}
#if TRACE >= 3
	fputs("\n", stderr);
#endif // TRACE
	return gmats;
}

RawNmi Collection::nmi(const Collection& cn1, const Collection& cn2, bool expbase)
{
	RawNmi  rnmi1;
	if(!cn1.clsnum() || !cn2.clsnum())
		return rnmi1;

	rnmi1 = cn1.nmi(cn2, expbase);
#if VALIDATE >= 2
	// Check NMI value for the inverse order of collections
	auto rnmi2 = cn2.nmi(cn1, expbase);
	fprintf(stderr, "nmi(), mi1: %G, mi2: %G,  dmi: %G\n", rnmi1.mi, rnmi2.mi
		, rnmi1.mi - rnmi2.mi);
	assert(equal(rnmi1.mi, rnmi2.mi, (cn1.clsnum() + cn2.clsnum()) / 2)
		&& "nmi(), rnmi is not symmetric");
#endif // VALIDATE
	return rnmi1;
}

RawNmi Collection::nmi(const Collection& cn, bool expbase) const
{
#if VALIDATE >= 2
	// Note: the processing is also fine (but redundant) for the empty collections
	assert(clsnum() && cn.clsnum() && "nmi(), non-empty collections expected");
#endif // VALIDATE
//	using ClustersMatching = SparseMatrix<Cluster*, Id>;  //!< Clusters matching matrix
//	using IndexedCounts = unordered_map<Cluster*, Id>;  //!< Indexed counts of the clusters
	using ClustersMatching = SparseMatrix<Cluster*, AccProb>;  //!< Clusters matching matrix

	// Reset member contributions if not zero
	auto initconts = [](const Collection& cn) {
		if(!cn.m_contsum)  // Note: ! is fine here
			return;
		for(const auto& cl: cn.m_cls)
			cl->mbscont = 0;
		cn.m_contsum = 0;
	};
	initconts(*this);
	initconts(cn);

//	// Traverse all members (nodes) and evaluate their contribution to each cluster.
//	// Total contribution should be equal to the number of nodes
//	auto mbsconts = [](const Collection& cn) {
//		for(auto& ncs: cn.m_ndcs) {
//			// Note: in case of fuzzy (unequal) overlaps the shares are unequal and
//			// should be stored in the Collection (ncs clusters member or a map)
//			const AccProb  share = 1 / AccProb(ncs.second.size());
//			for(auto cl: ncs.second)
//				cl->mbscont += share;
//		}
//	};
////	AccProb  c1sum = m_ndcs.size();
////	AccProb  c2sum = cn.m_ndcs.size();
//	//mbsconts(*this);
//	mbsconts(cn);

	ClustersMatching  clsmm = ClustersMatching(clsnum());  // Note: default max_load_factor is 1
	// Total sum of all values of the clsmm matrix, i.e. the number of
	// member nodes in both collections
	AccProb  cmmsum = 0;
//	//for



	// Traverse all clusters in the collection filling clusters matching matrix
	// and aggregated number of matches for each cluster in the collections (rows, cols)
	for(const auto& cl: m_cls) {
		// Traverse all members (node ids)
		for(auto nid: cl->members) {
			const auto imcls = cn.m_ndcs.find(nid);
// NOTE: this automatically synchronizes node bases without any penalty ?
			// Consider the case of unequal node base skipping the missed node,
			// i.e. sync the node base. An alternative solution is penalization
			// for the missed nodes assigning all the nodes present only in one
			// collection to an additional new cluster
			if(imcls == cn.m_ndcs.end())
				continue;
			const auto csnum = m_ndcs.at(nid).size();
			const Prob  cpart = Prob(1)/csnum;
			cl->mbscont += cpart;  // Note: contains only POSITIVE values
			cmmsum += cpart;
//			const auto imcls = cn.m_ndcs.find(nid);
//			// Consider the case of unequal node base skipping the missed node,
//			// i.e. sync the node base. An alternative solution is penalization
//			// for the missed nodes assigning all the nodes present only in one
//			// collection to an additional new cluster
//			if(imcls == cn.m_ndcs.end())
//				continue;
			// Find Matching clusters (containing the same member node id) in the foreign collection
			const auto  mcsnum = imcls->second.size();
#if VALIDATE >= 2
			assert(mcsnum && "nmi(), clusters should be present");
			Prob  cvsum = 0;
#endif // VALIDATE
			//const Prob  mcpart = Prob(1)/mcsnum;
			const AccProb  mmpart = AccProb(cpart) / mcsnum;  // Contribution to the whole matrix
			for(auto mcl: imcls->second) {
#if VALIDATE >= 2
				cvsum += mmpart;
#endif // VALIDATE
				clsmm(cl.get(), mcl) += mmpart;  // Note: contains only POSITIVE values
				mcl->mbscont += mmpart;  // Note: contains only POSITIVE values
			}
#if VALIDATE >= 2
			if(!equal<Prob>(cvsum, cpart, csnum*mcsnum)) {
				fprintf(stderr, "nmi(), cvsum: %G != %G for %lu csnum %lu mcslnum\n"
					, cvsum, cpart, csnum, mcsnum);
				assert(0 && "nmi(), accumulated value is invalid");
			}
#endif // VALIDATE
		}
	}

	RawNmi  rnmi;  // Resulting raw nmi, initially equal to 0
	if(clsmm.empty()) {
		fputs("WARNING nmi(), collection nodes have no any intersection"
			", the collections are totally different", stderr);
		return rnmi;
	}

//	// Traverse all clusters in the collection filling clusters matching matrix
//	// and aggregated number of matches for each cluster in the collections (rows, cols)
//	for(const auto& cl: m_cls) {
//		// Traverse all members (node ids)
//		for(auto nid: cl->members) {
//			// Find Matching clusters (containing the same member node id) in the foreign collection
//			const auto imcls = cn.m_ndcs.find(nid);
//			// Consider the case of unequal node base, i.e. missed node
//			if(imcls == cn.m_ndcs.end())
//				continue;
//			const auto  mcsnum = imcls->second.size();
//#if VALIDATE >= 2
//			assert(mcsnum && "nmi(), clusters should be present");
//#endif // VALIDATE
//			c1icts[cl.get()] += mcsnum;  // Note: contains only POSITIVE values
//			cmmsum += mcsnum;
//			for(auto mcl: imcls->second) {
//				++clsmm(cl.get(), mcl);  // Note: contains only POSITIVE values
//				++c2icts[mcl];  // Note: contains only POSITIVE values
//			}
//		}
//	}
//
//	// Traverse over all nodes to have equal contribution from overlapping and non-overlapping
//	// nodes to each cluster, otherwise overlapping nodes contributes k times more
//	// Note: in case of unequal node base use the smallest set of vertices because anyway
//	// the non-matching nodes are not considered
//	//
//	// For the overlapping nodes should we add 1 / ownerClsNum, or just 1?
//	// 	<=> Should we decrease per-cluster contribution of each overlapping node,
//	// 	or decrease a bit relative weight of all nodes considering overlapping parts
//	// 	as new nodes?
//	// Partial membership seems to be more fair, i.e. 1 / ownerClsNum contribution
//	// in case of overlaps, but full contribution should be in case of the multi-
//	// resolution clusters. So it should be parameterized.
//	// What should we do in case of flattened multi-resolution collection with
//	// overlapping clusters? - be biased to the resolutions, which include much
//	// more members than overlaps. => Use equal contribution from each node by default,
//	// which is not the precise (but rather accurate approximation) in case of
//	// overlapping clusters on a single resolution.
//	auto& ndcls = m_ndcs.size() <= cn.ndsnum() ? m_ndcs: cn.m_ndcs;
//	auto& mcn = m_ndcs.size() <= cn.ndsnum() ? cn: *this;
//	for(auto& ndc: ndcls) {
//		// Find clusters corresponding to this node in the cn
//		const auto imcls = mcn.m_ndcs.find(ndc.first);
//		// Consider the case of unequal node base, i.e. missed node
//		if(imcls == mcn.m_ndcs.end())
//			continue;
//		const auto  mcsnum = imcls->second.size();
//#if VALIDATE >= 2
//		assert(mcsnum && "nmi(), clusters should be present");
//#endif // VALIDATE
//		cmmsum += mcsnum * ndc.second.size();
//		// Traverse all clusters of the node
//		for(auto cl: ndc.second) {
//			c1icts[cl] += mcsnum;  // Note: contains only POSITIVE values
//			for(auto mcl: imcls->second) {
//				++clsmm(cl, mcl);
//				++c2icts[mcl];  // Note: contains only POSITIVE values
//			}
//		}
//	}

#if VALIDATE >= 2
	// Validate sum of vector counts to evaluate probabilities
	// Note: to have symmetric values normalization should be done by the max values in the row / col
	//assert(cmmsum % 2 == 0 && "nmi(), cmmsum should always be even");
#if TRACE >= 2
	#define TRACING_CLSCOUNTS_
	fprintf(stderr, "nmi(), cls1 counts (%lu): ", m_cls.size());
#endif // TRACE
	for(const auto& c1: m_cls) {
		m_contsum += c1->mbscont;
#ifdef TRACING_CLSCOUNTS_
		fprintf(stderr, " %G", c1->mbscont);
#endif // TRACING_CLSCOUNTS_
	}

#ifdef TRACING_CLSCOUNTS_
	fprintf(stderr, "\nnmi(), cls2 counts (%lu): ", cn.m_cls.size());
#endif // TRACING_CLSCOUNTS_
	for(const auto& c2: cn.m_cls) {
		cn.m_contsum += c2->mbscont;
#ifdef TRACING_CLSCOUNTS_
		fprintf(stderr, " %G", c2->mbscont);
#endif // TRACING_CLSCOUNTS_
	}
#ifdef TRACING_CLSCOUNTS_
	fputs("\n", stderr);
#endif // TRACING_CLSCOUNTS_

	if(!equal<AccProb>(m_contsum, cmmsum, m_cls.size())
	&& !equal<AccProb>(cn.m_contsum, cmmsum, cn.m_cls.size())) {  // Note: cmmsum should much to either of the sums
		fprintf(stderr, "nmi(), c1csum: %G, c2csum: %G, cmmsum: %G\n", m_contsum, cn.m_contsum, cmmsum);
		assert(0 && "nmi(), rows accumulation is invalid");
	}
#endif // VALIDATE

	// Evaluate probabilities using the clusters matching matrix
#if TRACE >= 2
	fprintf(stderr, "nmi(), nmi is evaluating with base %c, %G contributed members / nodes\n"
		, expbase ? 'e' : '2', cmmsum);
#endif // TRACE
	AccProb (*const clog)(AccProb) = expbase ? log : log2;  // log to be used
#if VALIDATE >= 2
	AccProb  psum = 0;  // Sum of probabilities over the whole matrix
#endif // VALIDATE
	AccProb  h12 = 0;  // Accumulated mutual probability over the matrix, I(cn1, cn2) in exp base
	AccProb  h1 = 0;  // H(cn1) - information size of the cn1 in exp base
	// Traverse matrix of the mutual information evaluate total mutual probability
	// and mutual probabilities for each collection
#if TRACE >= 2
	#define TRACING_CLSMM_  // Note: local / private macroses are ended with '_'
	fprintf(stderr, "nmi(), clsmm:\n");
#endif // TRACE
	for(const auto& icm: clsmm) {
		const auto  c1cnorm = icm.first->mbscont;  // Accumulated value of the current cluster from cn1
		// Evaluate information size of the current cluster in the cn1
		const AccProb  cprob = AccProb(c1cnorm) / cmmsum;
#if VALIDATE >= 2
		assert(cprob > 0 && cprob <= 1 && "nmi(), invalid probability");
#endif // VALIDATE
		h1 -= cprob * clog(cprob);

		// Travers row
#ifdef TRACING_CLSMM_
		fprintf(stderr, "%.3G:  ", c1cnorm);
#endif // TRACING_CLSMM_
// TODO: Use full formula and ? max match to consider overlaps
		for(auto& icmr: icm.second) {
#if VALIDATE >= 2
			assert(icmr.val > 0 && "nmi(), matrix of clusters matching should contain only positive values");
#endif // VALIDATE
#ifdef TRACING_CLSMM_
			fprintf(stderr, " %G[%.3G]", icmr.val, icmr.pos->mbscont);
#endif // TRACING_CLSMM_
			// Evaluate mutual probability of the cluster (divide by multiplication of counts of both clusters)
			// Note: in the original NMI: AccProb(icmr.val) / nodesNum [ = cmmsum]
			AccProb  mcprob = AccProb(icmr.val) / cmmsum;  // (c1cnorm * AccId(icmr.pos->mbscont));  // This evaluation is used in gecmi
			//fprintf(stderr, "nmi(),  mcprob: %G (val: %G), v1: %G, v2: %G\n"
			//	, mcprob, icmr.val, c1cnorm, icmr.pos->mbscont);
#if VALIDATE >= 2
			psum += mcprob;
			assert(mcprob > 0 && mcprob <= 1 && icmr.pos->mbscont > 0
				&& "nmi(), invalid probability or multiplier");
#endif // VALIDATE
			// Accumulate total normalized mutual information
			// Note: e base is used instead of 2 to have absolute entropy instead of bits length
			//const auto lval = icmr.val / (icmr.pos->mbscont * cprob);  // cprob; c1cnorm
			//mi += mcprob * clog(lval);  // mi = h1 + h2 - h12;  Note: log(a/b) = log(a) - log(b)
			h12 -= mcprob * clog(mcprob);  // clog(lval);  // Note: log(a/b) = log(a) - log(b)
//#ifndef TRACING_CLSMM_
//			fprintf(stderr, "  %G * clog(%G [%G / (%G * %G)]) = %G\n", mcprob, lval
//				, icmr.val, icmr.pos->mbscont, cprob, mcprob * clog(lval));
//#endif // TRACING_CLSMM_
		}
#ifdef TRACING_CLSMM_
		fputs("\n", stderr);
#endif // TRACING_CLSMM_
	}
#if VALIDATE >= 2
	fprintf(stderr, "nmi(), psum: %G\n", psum);
	assert(equal(psum, 1., cmmsum) && "nmi(), total probability of the matrix should be 1");
#endif // VALIDATE

	// Evaluate information size cn2 clusters
	AccProb  h2 = 0;  // H(cn2) - information size of the cn1 in exp base
	for(const auto& c2: cn.m_cls) {
		AccProb  cprob = AccProb(c2->mbscont) / cmmsum;
#if VALIDATE >= 2
		assert(cprob > 0 && cprob <= 1 && "nmi() 2, invalid probability");
#endif // VALIDATE
		h2 -= cprob * clog(cprob);
	}

	rnmi(h1 + h2 - h12, h1, h2, Evaluation::OVERLAPPING);
#if TRACE >= 2
	Prob  nmix = (h1 + h2 - h12) / (h1 >= h2 ? h1 : h2);
	fprintf(stderr, "nmi(),  mi: %G,  h1: %G, h2: %G,  NMI_max: %G\n", rnmi.mi, h1, h2, nmix);
#endif // TRACE

	return rnmi;
}
