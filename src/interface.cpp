//! \brief Extrinsic measures evaluation interface implementation.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-12-15

#include <cstdio>
//#include <bitset>
#include "interface.h"


using namespace daoc;

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

string to_string(F1 f1)
{
	// Convert to semantic string
	string  val;
	switch(f1) {
	case F1::PARTPROB:
		val = "PARTPROB";
		break;
	case F1::HARMONIC:
		val = "HARMONIC";
		break;
	case F1::STANDARD:
		val = "STANDARD";
		break;
	case F1::NONE:
	default:
		val = "NONE";
	}
	return val;
}

string to_string(Match mkind)
{
	// Convert to semantic string
	string  val;
	switch(mkind) {
	case Match::WEIGHTED:
		val = "WEIGHTED";
		break;
	case Match::UNWEIGHTED:
		val = "UNWEIGHTED";
		break;
	case Match::COMBINED:
		val = "COMBINED";
		break;
	case Match::NONE:
	default:
		val = "NONE";
	}
	return val;
}

NodeBase NodeBase::load(const char* filename, float membership, ::AggHash* ahash
	, size_t cmin, size_t cmax, bool verbose)
{
	NodeBase  nb;  // Return using NRVO optimization
	NamedFileWrapper  finp(filename, "r");
	if(finp)
		static_cast<UniqIds&>(nb) = loadNodes<Id, AccId>(finp, membership, ahash
			, cmin, cmax, verbose);
	else perror((string("WARNING load(), can't open ") += filename).c_str());

	return nb;
}

bool xwmatch(Match m) noexcept
{
	return m == Match::WEIGHTED || m == Match::COMBINED;
}


bool xumatch(Match m) noexcept
{
	return m == Match::UNWEIGHTED || m == Match::COMBINED;
}

// Accessory functions ---------------------------------------------------------
AccProb hmean(AccProb a, AccProb b) noexcept
{
	static_assert(is_floating_point<AccProb>::value, "AccProb should be a floating point type");
	// Note: both a = b = 0 and a = -b are considered and yield 0
	return a + b != 0 ? 2 * a / (a + b) * b : 0;
}

AccProb gmean(AccProb a, AccProb b) noexcept
{
#ifdef DEBUG
	assert(a >= 0 && b >= 0 && "gmean(), the probabilities should E [0, 1]");
#endif // DEBUG
	return sqrt(a * b);
}

AccProb amean(AccProb a, AccProb b) noexcept
{
	return (a + b) / 2;
}
