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
//#include <bitset>
//#include <cmath>  // sqrt
#include "operations.hpp"
#include "interface.h"


using std::out_of_range;
//using std::bitset;
using std::min;
using std::max;
using namespace daoc;

// Cluster definition ----------------------------------------------------------
template <typename Count>
Cluster<Count>::Cluster(): members(), counter(), mbscont()
{
#if VALIDATE >= 1
	assert(!mbscont && "Cluster(), contin should be 0");  // Note: ! is fine here
#endif // VALIDATE
}

//string to_string(Evaluation eval, bool bitstr)
//{
//	static_assert(sizeof(Evaluation) == sizeof(EvalBase)
//		, "to_string(), Evaluation type must be the same size as EvalBase");
//	// Convert to bit string
//	if(bitstr)
//		return bitset<sizeof(Evaluation) * 8>(static_cast<EvalBase>(eval))
//			.to_string().insert(0, "0b");
//
//	// Convert to semantic string
//	string  val;
//	switch(eval) {
//	case Evaluation::MULTIRES:
//		val = "MULTIRES";
//		break;
//	case Evaluation::OVERLAPPING:
//		val = "OVERLAPPING";
//		break;
//	case Evaluation::MULRES_OVP:
//		val = "MULRES_OVP";
//		break;
//	case Evaluation::NONE:
//	default:
//		val = "NONE";
//	}
//	return val;
//}

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

// Collection definitions ------------------------------------------------------
NodeBase NodeBase::load(const char* filename, float membership, ::AggHash* ahash
	, size_t cmin, size_t cmax)
{
	NodeBase  nb;  // Return using NRVO optimization
	NamedFileWrapper  finp(filename, "r");
	if(finp)
		static_cast<UniqIds&>(nb) = loadNodes<Id, AccId>(finp, membership, ahash, cmin, cmax);
	else perror((string("WARNING load(), can't open ") += filename).c_str());

	return nb;
}

template <typename Count>
Collection<Count> Collection<Count>::load(const char* filename, float membership, ::AggHash* ahash
	, const NodeBaseI* nodebase)
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
	::AggHash  mbhash;  // Nodes hash (only unique nodes, not all the members)
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
		auto icl = cn.m_cls.emplace(new Cluster<Count>()).first;
		Cluster<Count>* const  pcl = icl->get();
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
			// Filter out nodes if required
			if(nodebase && !nodebase->nodeExists(nid))
				continue;
			members.push_back(nid);
			auto& ncs = cn.m_ndcs[nid];
			// Update hash if required
			if(ncs.empty())
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
	// Assign hash to the results
	cn.m_ndshash = mbhash.hash();  // Note: required to identify the unequal node base in processing collections
	if(ahash)
		*ahash = move(mbhash);
#if TRACE >= 2
	printf("loadNodes() [shrinked], loaded %lu clusters (reserved %lu buckets, overhead: %0.2f %%) and"
		" %lu nodes (reserved %lu buckets, overhead: %0.2f %%) with hash %lu from %s\n"
		, cn.m_cls.size(), cn.m_cls.bucket_count()
		, cn.m_cls.size() ? float(cn.m_cls.bucket_count() - cn.m_cls.size()) / cn.m_cls.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndcs.size(), cn.m_ndcs.bucket_count()
		, cn.m_ndcs.size() ? float(cn.m_ndcs.bucket_count() - cn.m_ndcs.size()) / cn.m_ndcs.size() * 100
			: numeric_limits<float>::infinity()
		, cn.m_ndshash, file.name().c_str());
#elif TRACE >= 1
	printf("Loaded %lu clusters %lu nodes from %s\n", cn.m_cls.size()
		, cn.m_ndcs.size(), file.name().c_str());
#endif

	return cn;
}

template <typename Count>
Prob Collection<Count>::f1gm(const CollectionT& cn1, const CollectionT& cn2, bool weighted
	, bool prob)
{
	// Initialized accessory data for evaluations
	if(is_floating_point<Count>::value && !(cn1.m_contsum && cn2.m_contsum)) {  // Note: strict ! is fine here
		// Evaluate members contributions
        //! \brief Initialized cluster members contributions
        //!
        //! \param cn const CollectionT&  - target collection to initialize cluster
        //! members contributions
        //! \return void
		constexpr auto initconts = [](const CollectionT& cn) noexcept {
			for(auto& ndcs: cn.m_ndcs) {
				// ATTENTION: in case of fuzzy (unequal) overlaps the shares are unequal and
				// should be stored in the Collection (ncs clusters member or a map)
				const AccProb  ndshare = AccProb(1) / ndcs.second.size();
				for(auto cl: ndcs.second)
					cl->mbscont += ndshare;
			}
			// Mark that mbscont of clusters are used
			cn.m_contsum = -1;
		};
		initconts(cn1);
		initconts(cn2);
	}

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

template <typename Count>
AccProb Collection<Count>::avggms(const CollectionT& cn, bool weighted, bool prob) const
{
	AccProb  accgm = 0;
	const auto  gmats = gmatches(cn, prob);
	if(weighted) {
		const Id  csnum = m_cls.size();
#if VALIDATE >= 2
		assert(gmats.size() == csnum
			&& "avggms(), matches are not synchronized with the clusters");
#endif // VALIDATE
		AccCont  csizesSum = 0;
		auto icl = m_cls.begin();
		for(size_t i = 0; i < csnum; ++i) {
			// Evaluate members considering their shared contributions
			// ATTENTION: F1 compares clusters per-pair, so it is much simpler and
			// has another semantics of contribution for the multi-resolution case
			AccCont  ccont = m_overlaps ? (*icl)->cont() : (*icl)->members.size();
#if VALIDATE >= 2
			assert(ccont > 0 && "avggms(), the contribution should be positive");
#endif // VALIDATE
			++icl;
			accgm += gmats[i] * ccont;
			csizesSum += ccont;
		}
#if VALIDATE >= 2
		assert(!less<AccCont>(csizesSum, csnum, csnum) && "avggms(), invalid sum of the cluster sizes");
#endif // VALIDATE
		accgm /= csizesSum / AccProb(csnum);
	} else for(auto gm: gmats)
		accgm += gm;
#if VALIDATE >= 1
	if(less<AccProb>(gmats.size(), accgm, gmats.size()))
		throw std::overflow_error("avggms(), accgm is invalid (larger than"
			" the number of clusters)\n");
#endif // VALIDATE
	return accgm / gmats.size();
}

template <typename Count>
Probs Collection<Count>::gmatches(const CollectionT& cn, bool prob) const
{
	// Greatest matches (Max F1 or partial probability) for each cluster [of this collection, self];
	Probs  gmats;  // Uses NRVO return value optimization
	gmats.reserve(m_cls.size());  // Preallocate the space

	// Whether overlapping or multi-resolution clusters are evaluated
	auto fmatch = prob ? &Cluster<Count>::pprob : &Cluster<Count>::f1;  // Function evaluating value of the match

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
				if(m_overlaps)
					// In case of overlap contributes the smallest share (of the largest number of owners)
					mcl->counter(cl.get(), AccProb(1)
						/ max(m_ndcs.at(nid).size(), cn.m_ndcs.at(nid).size()));
				else mcl->counter(cl.get(), 1);
				// Note: only the max value for match is sufficient
				// ATTENTION: F1 compares clusters per-pair, so it is much simpler and
				// has another semantics of contribution for the multi-resolution case
				const Prob  match = ((*cl).*fmatch)(mcl->counter(), m_overlaps
					? mcl->cont() : mcl->members.size());
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

template <typename Count>
RawNmi Collection<Count>::nmi(const CollectionT& cn1, const CollectionT& cn2, bool expbase)
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

template <typename Count>
RawNmi Collection<Count>::nmi(const CollectionT& cn, bool expbase) const
{
	ClustersMatching  clsmm;  //  Clusters matching matrix
	AccCont  cmmsum = evalconts(cn, &clsmm);  // Sum of all values of the clsmm

	RawNmi  rnmi;  // Resulting raw nmi, initially equal to 0
	if(clsmm.empty()) {
		fputs("WARNING nmi(), collection nodes have no any intersection"
			", the collections are totally different", stderr);
		return rnmi;
	}

	// Evaluate probabilities using the clusters matching matrix
	AccProb (*const clog)(AccProb) = expbase ? log : log2;  // log to be used

    //! \brief Information content
    //! \pre val > 0, capacity >= val
    //!
    //! \param val AccProb  - contributing value
    //! \param capacity AccProb  - total capacity
    //! \return AccProb  - resulting information content H(val, capacity)
	auto infocont = [clog](AccProb val, AccProb capacity) -> AccProb {
#if VALIDATE >= 2
		assert(val > 0 && capacity >= val  // && prob > 0 && prob <= 1
			&& "infocont(), invalid input values");
#endif // VALIDATE
		//if(!val)
		//	return 0;
		const AccProb  prob = val / capacity;
		return prob * clog(prob);
	};

	AccProb  h12 = 0;  // Accumulated mutual probability over the matrix, I(cn1, cn2) in exp base
	AccProb  h1 = 0;  // H(cn1) - information size of the cn1 in exp base
	// Traverse matrix of the mutual information evaluate total mutual probability
	// and mutual probabilities for each collection
#if VALIDATE >= 2
	AccProb  psum = 0;  // Sum of probabilities over the whole matrix
#endif // VALIDATE
#if TRACE >= 2
	fprintf(stderr, "nmi(), nmi is evaluating with base %c, clsmatch matrix sum: %.3G\n"
		, expbase ? 'e' : '2', AccProb(cmmsum));

	#define TRACING_CLSMM_  // Note: local / private macroses are ended with '_'
	fprintf(stderr, "nmi(), clsmm:\n");
#endif // TRACE

	// TODO: To evaluate overlapping or multi-resolution NMI the clusters matching
	// matrix should be renormalized considering best matches and remaining parts
	// (the last part is missed in onmi, which caused it's inapplicability for
	// the multi-resolution evaluations).
	// Order collections by the number of clusters, find best matches to the
	// collection with the largest number of clusters and renormalize the matrix
	// to maximize the match resolving overlaps (or retaining if required).
	// The idea is to avoid NMI penalization of the overlaps by renormalizining
	// overlaps A,B : A',B' as A:A', B:B' moving to them contribution from A:B'
	// and B:A'.
	// The hard part is renormalization in case of the partial match with overlaps
	// in each part.

	// Evaluate NMI (standard NMI if cmmsum is not renormalized)
#if VALIDATE >= 2
	assert(m_contsum > 0 && cn.m_contsum > 0
		&& "nmi(), collection clusters contribution is invalid");
#endif // VALIDATE
	for(const auto& icm: clsmm) {
		// Evaluate information size (content) of the current cluster in the cn1
		// infocont(Accumulated value of the current cluster from cn1, the number of nodes)
		h1 -= infocont(icm.first->cont(), m_contsum);  // ndsnum(), cmmsum

		// Travers row
#ifdef TRACING_CLSMM_
		fprintf(stderr, "%.3G:  ", AccProb(icm.first->cont()));
#endif // TRACING_CLSMM_
		for(auto& icmr: icm.second) {
#if VALIDATE >= 2
			assert(icmr.val > 0 && "nmi(), matrix of clusters matching should contain only positive values");
#endif // VALIDATE
#ifdef TRACING_CLSMM_
			fprintf(stderr, " %G[%.3G]", AccProb(icmr.val), AccProb(icmr.pos->cont()));
#endif // TRACING_CLSMM_
#if VALIDATE >= 2
			// Evaluate mutual probability of the cluster (divide by multiplication of counts of both clusters)
			psum += AccProb(icmr.val) / cmmsum;
#endif // VALIDATE
			// Accumulate total normalized mutual information
			// Note: e base is used instead of 2 to have absolute entropy instead of bits length
			//const auto lval = icmr.val / (icmr.pos->cont() * cprob);  // cprob; icm.first->mbscont
			//mi += mcprob * clog(lval);  // mi = h1 + h2 - h12;  Note: log(a/b) = log(a) - log(b)

			// Note: in the original NMI: AccProb(icmr.val) / nodesNum [ = cmmsum]
			h12 -= infocont(icmr.val, cmmsum);
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
	for(const auto& c2: cn.m_cls)
		h2 -= infocont(c2->cont(), cn.m_contsum);  // cn.ndsnum(), cmmsum

	rnmi(h1 + h2 - h12, h1, h2);  // h1 + h2 - h12;  h12
	//rnmi(h12, h1, h2);  // h1 + h2 - h12;  h12  // ATTENTION: this approach is invalid, it shows higher values for worse matching, but [and] awards overlaps

	// VVV Tracks only shapes of clusters, but not the clusters [members] matching between the collections => need h12
	//rnmi(min(h1, h2), h1, h2);  // h1 + h2 - h12;  h12  // ATTENTION: this approach is invalid
#if TRACE >= 2
	Prob  nmix = rnmi.mi / max(h1, h2);
	fprintf(stderr, "nmi(),  mi: %G (h12: %G),  h1: %G, h2: %G,  NMI_max: %G\n"
		, rnmi.mi, h12, h1, h2, nmix);
#endif // TRACE

	return rnmi;
}

template <typename Count>
auto Collection<Count>::evalconts(const CollectionT& cn, ClustersMatching* pclsmm) const -> AccCont
{
	// Skip evaluations if they already performed (the case of evaluating
	// overlapping F1 after NMI)
	if(!pclsmm && m_contsum && cn.m_contsum) {  // Note: strict values usage is fine here
#if TRACE >= 2
		fputs("evalconts(), contributions evaluation skipped as already performed\n", stderr);
#endif // TRACE
		return 0;
	}

#if VALIDATE >= 2
	// Note: the processing is also fine (but redundant) for the empty collections
	assert(clsnum() && cn.clsnum() && "evalconts(), non-empty collections expected");
#endif // VALIDATE

	// Reset member contributions if not zero
	clearconts();
	cn.clearconts();

    //! \brief Contribution from the member of clusters
    //!
    //! \param owners  - the number of owner clusters of the member
    //! \return AccProb - resulting contribution to a single owner
    // Note: in contribute equally to each upper cluster on each resolution, where
    // only one relevant cluster on each resolution should exists. Multi-resolution
    // structure should have the same node base of each resolution, otherwise
    // overlapping evaluation is more fair. In fact overlaps is generalization of
    // the multi-resolution evaluation, but can't be directly applied to the
    // case of multi-resolution overlapping case, where 2-level (by levels and
	// by overlaps in each level) evaluations are required.
	auto mbcont = [](size_t owners) -> AccCont {
		static_assert(!m_overlaps || is_floating_point<AccCont>::value
			, "mbcont(), invalid types");
		return m_overlaps ? AccCont(1) / owners : AccCont(1);
	};

    //! \brief  Update contribution of to the member-related clusters
    //!
    //! \param cls const ClusterPtrs&  - clusters to be updated
    //! \return AccProb  - contributing value (share in case of overlaps in a single resolution)
    // ATTENTION: Such evaluation is applicable only for non-fuzzy overlaps (equal sharing)
	auto updateCont = [mbcont](const ClusterPtrs<Count>& cls) -> AccCont {
		const AccCont  share = mbcont(cls.size());
		for(auto cl: cls)
			cl->mbscont += share;
		return share;
	};

	ClustersMatching  clsmm = ClustersMatching(clsnum());  // Note: default max_load_factor is 1
	// Total sum of all values of the clsmm matrix, i.e. the number of
	// member nodes in both collections
	AccProb  cmmsum = 0;
	// Evaluate contributions to the clusters of each collection and to the
	// mutual matrix of clusters matching
	// Note: in most cases collections has the same node base, or it was synchronized
	// explicitly in advance, so handle unequal node base as a rare case
	//
	// ATTENTION: For 2+ levels cmmsum equals mbsnum * levs_num, in case of overlaps
	// it also > mbsnum.
	// =>  .mbscont is ALWAYS required except the case of non-overlapping clustering
	// on a single resolution
	for(auto& ncs: m_ndcs) {
		// Note: always evaluate contributions to the clusters of this collection
		const AccProb  share1 = mbcont(ncs.second.size());
		// Evaluate contribution to the second collection if any
		auto incs2 = cn.m_ndcs.find(ncs.first);
		const auto  cls2num = incs2 != cn.m_ndcs.end() ? incs2->second.size() : 0;
		AccProb  share;
		if(cls2num)
			share = (m_overlaps ? updateCont(incs2->second)
				: mbcont(incs2->second.size())) * share1;  // Note: shares already divided by clsXnum
		for(auto cl: ncs.second) {
			if(m_overlaps)
				cl->mbscont += share1;
			// Update clusters matching matrix
			if(cls2num) {
				cmmsum += share * cls2num;
				for(auto cl2: incs2->second) {
					clsmm(cl, cl2) += share;  // Note: contains only POSITIVE values
					if(!m_overlaps) {
						cl->mbscont += share;
						cl2->mbscont += share;
					}
				}
			}
		}
	}
	// Consider the case of unequal node base, contribution from the missed nodes
	AccCont  econt = 0;  // Extra contribution from the cn
	if((m_ndshash && cn.m_ndshash && m_ndshash != cn.m_ndshash) || ndsnum() != cn.ndsnum()) {
		for(auto& ncs: cn.m_ndcs) {
			// Skip processed nodes
			if(m_ndcs.count(ncs.first))
				continue;
			econt += updateCont(ncs.second);
		}
	} else if(!m_ndshash || !cn.m_ndshash)
		fputs("WARNING evalconts(), collection(s) hashes were not evaluated (%lu, %lu)"
			", so some unequal nodes might be skipped on evaluation, which cause"
			" approximate results\n", stderr);

	// Set results
	if(pclsmm)
		*pclsmm = move(clsmm);
	// Set contributions
	m_contsum = cmmsum;
	cn.m_contsum = cmmsum + econt;

#if VALIDATE >= 2
	// Validate sum of vector counts to evaluate probabilities
	// Note: to have symmetric values normalization should be done by the max values in the row / col
	//assert(cmmsum % 2 == 0 && "nmi(), cmmsum should always be even");
#if TRACE >= 2
	#define TRACING_CLSCOUNTS_
	fprintf(stderr, "evalconts(), cls1 counts (%lu): ", m_cls.size());
#endif // TRACE
	m_contsum = 0;
	for(const auto& c1: m_cls) {
		m_contsum += c1->cont();
#ifdef TRACING_CLSCOUNTS_
		fprintf(stderr, " %.3G", AccProb(c1->cont()));
#endif // TRACING_CLSCOUNTS_
	}

#ifdef TRACING_CLSCOUNTS_
	fprintf(stderr, "\nevalconts(), cls2 counts (%lu): ", cn.m_cls.size());
#endif // TRACING_CLSCOUNTS_
	cn.m_contsum = 0;
	for(const auto& c2: cn.m_cls) {
		cn.m_contsum += c2->cont();
#ifdef TRACING_CLSCOUNTS_
		fprintf(stderr, " %.3G", AccProb(c2->cont()));
#endif // TRACING_CLSCOUNTS_
	}
#ifdef TRACING_CLSCOUNTS_
	fputs("\n", stderr);
#endif // TRACING_CLSCOUNTS_
	if(m_overlaps)  // consum equals to the number of nodes for the overlapping case
		assert(equal<AccCont>(m_contsum, ndsnum(), ndsnum())
			&& equal<AccCont>(cn.m_contsum, cn.ndsnum(), cn.ndsnum())
			&& "evalconts(), consum validation failed");
#endif // VALIDATE
#if VALIDATE >= 1
	if((//m_overlaps &&
	(!equal<AccProb>(m_contsum, cmmsum, m_cls.size())
	|| !equal<AccProb>(cn.m_contsum - econt, cmmsum, cn.m_cls.size())))
//	|| (!m_overlaps && !equal<AccProb>(max(m_contsum, cn.m_contsum - econt)
//		, cmmsum, m_cls.size()))
	) {  // Note: cmmsum should match to either of the sums
		fprintf(stderr, "evalconts(), c1csum: %.3G, c2csum: %.3G, cmmsum: %.3G\n"
			, AccProb(m_contsum), AccProb(cn.m_contsum), AccProb(cmmsum));
		throw domain_error(string("nmi(), rows accumulation is invalid")
			.append(m_overlaps ? "\n"
			: ", probably cluster overlaps occurred in the multi-resolution mode\n"));
	}
#endif // VALIDATE

	return cmmsum;
}

template <typename Count>
void Collection<Count>::clearconts() const noexcept
{
	// Reset member contributions if not zero
	if(!m_contsum)  // Note: ! is fine here
		return;
	for(const auto& cl: m_cls)
		cl->mbscont = 0;
	m_contsum = 0;
}
