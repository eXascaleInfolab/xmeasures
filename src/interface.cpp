//! \brief Extrinsic measures evaluation interface.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include "interface.h"


Clusters loadClustering(const char* filename, float membership)
{
	Clusters  cls;  // Return using NRVO, named return value optimization

	// Open file
	NamedFileWrapper  finp(filename, "r");
	if(!finp) {
		perror(string("ERROR loadClustering(), failed on opening ").append(filename).c_str());
		return cls;
	}
	// Load clusters


	// Note: AggHash can be used to verify that each loaded cluster is unique

	return cls;
}

float evalF1(const Clusters& cls1, const Clusters& cls2)
{
	return 0;
}

float evalNmi(const Clusters& cls1, const Clusters& cls2)
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
