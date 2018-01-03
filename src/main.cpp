//! \brief Extrinsic measures evaluation for overlapping multi-resolution clusterings
//! with possible unequal node base.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include <cstdio>
#include "cmdline.h"  // Arguments parsing
#include "macrodef.h"
#include "interface.hpp"


int main(int argc, char **argv)
{
	gengetopt_args_info  args_info;
	auto  err = cmdline_parser(argc, argv, &args_info);
	if(err)
		return err;

	// Validate required xmeasure
	if(!args_info.f1_given && !args_info.nmi_given) {
		fputs("WARNING, no any measures to evaluate specified\n", stderr);
		cmdline_parser_print_help();
		return EINVAL;
	}

	if(args_info.membership_arg <= 0) {
		fprintf(stderr, "ERROR, positive membership is expected: %G\n", args_info.membership_arg);
		return EDOM;
	}

	{	// Validate the number of input files
		// Note: sync_arg is specified if sync_given
		const auto  inpfiles = args_info.inputs_num + (args_info.sync_given || args_info.label_given);  // The number of input files
		if(inpfiles < 2 || inpfiles > 2 + args_info.sync_given + args_info.label_given) {
			fputs("ERROR, 2 input clusterings are required with possibly additional"
				" node base and clusters labels, i.e. 2-4 input files in total\n", stderr);
			cmdline_parser_print_help();
			return EINVAL;
		}
	}

	// Verify that labeled clusters correspond to the node base if any of them is specified
	if(args_info.sync_given && args_info.label_given && strcmp(args_info.sync_arg, args_info.label_arg))
		throw invalid_argument("ERROR, node base file should correspond to the labeled clusters if both are specified\m");

	// Load node base if required
	NodeBase  ndbase;
	::AggHash  nbhash;
	if(args_info.sync_given && args_info.inputs_num == 2)
		ndbase = NodeBase::load(args_info.sync_arg, args_info.membership_arg, &nbhash
			, 0, 0, args_info.detailed_flag);

	auto process = [&](auto evaluation) -> int {
		using Count = decltype(evaluation);
		using Collection = Collection<Count>;
		// Load collections as relations
		::AggHash  cn1hash, cn2hash;
		// Note: cn1 is nodebase if specified and not in the separated file
		const bool  cn1base = (args_info.sync_given || args_info.label_given) && args_info.inputs_num < 2;
		//const char*  nbfile = args_info.sync_given
		auto cn1 = Collection::load(cn1base ? args_info.sync_given ? args_info.sync_arg
			: args_info.label_arg : args_info.inputs[0]
			, args_info.membership_arg, &cn1hash
			, ndbase ? &ndbase : nullptr, args_info.detailed_flag);
		if(ndbase) {
			if(nbhash != cn1hash) {
				fprintf(stderr, "ERROR, nodebase hash %lu (%lu nodes) != filtered"
					" collection nodes hash %lu (%lu)\n", nbhash.hash(), nbhash.size()
					, cn1hash.hash(), cn1hash.size());
				return EINVAL;
			}
			ndbase.clear();
		}
		auto cn2 = Collection::load(args_info.inputs[!cn1base], args_info.membership_arg, &cn2hash
			, args_info.sync_given ? &cn1 : nullptr, args_info.detailed_flag);

		if(!cn1.ndsnum() || ! cn2.ndsnum()) {
			fprintf(stderr, "WARNING, at least one of the collections is empty, there is nothing"
				" to evaluate. Collection nodes sizes: %u, %u\n", cn1.ndsnum(), cn2.ndsnum());
			return EINVAL;
		}

		// Check the collections' nodebase
		if(cn1hash != cn2hash) {
			fprintf(stderr, "WARNING, the nodes in the collections differ: %u nodes"
				" with hash %lu, size: %lu, ids: %lu, id2s: %lu) != %u nodes with hash %lu"
				", size: %lu, ids: %lu, id2s: %lu);  synchronize: %s, label: %s\n"
				, cn1.ndsnum(), cn1hash.hash(), cn1hash.size(), cn1hash.idsum(), cn1hash.id2sum()
				, cn2.ndsnum(), cn2hash.hash(), cn2hash.size(), cn2hash.idsum(), cn2hash.id2sum()
				, daoc::toYesNo(args_info.sync_given), daoc::toYesNo(args_info.label_given));
			if(args_info.sync_given) {
				fputs("ERROR, the nodes base had to be synchronized\n", stderr);
				return EINVAL;
			}
		}

		// Evaluate and output measures
		// Note: evaluation of overlapping F1 after NMI allows to reuse some
		// calculations, for other cases the order of evaluations does not matter
		puts(string("= ").append(is_floating_point<Count>::value
			? "Overlaps" : "Multi-resolution").append(" Evaluation =").c_str());
		if(args_info.nmi_given) {
			auto rnmi = Collection::nmi(cn1, cn2, args_info.ln_flag, args_info.detailed_flag);
			// Set NMI to NULL if collections have no any mutual information
			// ATTENTION: for some cases, for example when one of the collections is a single cluster,
			// NMI will always yield 0 for any clusters in the second collection, which is limitation
			// of the original NMI measure. Similar issues possible in more complex configurations.
			if(rnmi.mi <= precision_limit<decltype(rnmi.mi)>()) {  // Note: strict ! is fine here
				throw domain_error("NMI is not applicable to the specified collections: 0, which says nothing about the similarity\n");
				rnmi.h1 = rnmi.h2 = 1;
			}
			const auto  nmix = rnmi.mi / std::max(rnmi.h1, rnmi.h2);
			if(args_info.all_flag)
				printf("NMI_max: %G, NMI_sqrt: %G, NMI_avg: %G, NMI_min: %G\n"
					, nmix, rnmi.mi / sqrt(rnmi.h1 * rnmi.h2)
					, 2 * rnmi.mi / (rnmi.h1 + rnmi.h2)
					, rnmi.mi / std::min(rnmi.h1, rnmi.h2));
			else printf("NMI_max:\n%G\n", nmix);
		}
		if(args_info.f1_given) {
			// Assign required F1 type
			F1  f1kind = F1::NONE;
			// Note: args_info.f1_orig is empty if default value is used
			char  f1suf = '-';  // Suffix char of the selected F1 measure
			switch(args_info.f1_arg) {
			case f1_arg_partprob:
				f1kind = F1::PARTPROB;
				f1suf = 'p';
				break;
			case f1_arg_harmonic:
				f1kind = F1::HARMONIC;
				f1suf = 'h';
				break;
			case f1_arg_standard:
				f1kind = F1::STANDARD;
				f1suf = 's';
				break;
			default:
				throw invalid_argument("main(), UNKNOWN F1 policy specified\n");
			}
			// Assign matching kind
			Match  mkind = Match::NONE;
			// Note: args_info.kind_orig is empty if default value is used
			char  kindsuf = '-';  // Suffix char of the selected F1 measure
			switch(args_info.kind_arg) {
			case kind_arg_weighted:
				mkind = Match::WEIGHTED;
				kindsuf = 'w';
				break;
			case kind_arg_unweighed:
				mkind = Match::UNWEIGHTED;
				kindsuf = 'u';
				break;
			case kind_arg_combined:
				mkind = Match::COMBINED;
				kindsuf = 'c';
				break;
			default:
				throw invalid_argument("main(), UNKNOWN Matching policy specified\n");
			}

			//if(args_info.nmi_flag)
			//	fputs("; ", stdout);
			printf("F1%c_%c (%s, %s):\n%G\n", f1suf, kindsuf, to_string(f1kind).c_str(), to_string(mkind).c_str()
				, Collection::f1(cn1, cn2, f1kind, mkind, args_info.detailed_flag));
		}

		return 0;
	};


    return args_info.ovp_flag ? process(AccProb()) : process(Id());
}
