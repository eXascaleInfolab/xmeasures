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
#include "interface.h"


int main(int argc, char **argv)
{
	gengetopt_args_info  args_info;
	auto  err = cmdline_parser(argc, argv, &args_info);
	if(err)
		return err;

	// Validate arguments
	if(!args_info.f1f1_given && !args_info.f1pp_given && !args_info.nmi_given) {
		fputs("WARNING, no any measures to evaluate specified\n", stderr);
		cmdline_parser_print_help();
		return 1;
	}

	if(args_info.inputs_num != 2) {
		fputs("ERROR, 2 input clusterings are required\n", stderr);
		cmdline_parser_print_help();
		return 1;
	}

	if(args_info.membership_arg <= 0) {
		fprintf(stderr, "ERROR, positive membership is expected: %G\n", args_info.membership_arg);
		return 1;
	}

	// Load collections as relations
	AggHash  cn1hash, cn2hash;
	auto cn1 = Collection::load(args_info.inputs[0], &cn1hash, args_info.membership_arg);
	auto cn2 = Collection::load(args_info.inputs[1], &cn2hash, args_info.membership_arg);

	if(!cn1.ndsnum() || ! cn2.ndsnum()) {
		fprintf(stderr, "WARNING, at least one of the collections is empty, there is nothing"
			" to evaluate. Collection nodes sizes: %u, %u\n", cn1.ndsnum(), cn2.ndsnum());
		return 1;
	}

	// Check the nodebase
	if(cn1hash != cn2hash)
		fprintf(stderr, "WARNING, the nodes in the collections differ: %u nodes"
			" with hash %lu, size: %lu, ids: %lu, id2s: %lu) != %u nodes with hash %lu"
			", size: %lu, ids: %lu, id2s: %lu)\n"
			, cn1.ndsnum(), cn1hash.hash(), cn1hash.size(), cn1hash.idsum(), cn1hash.id2sum()
			, cn2.ndsnum(), cn2hash.hash(), cn2hash.size(), cn2hash.idsum(), cn2hash.id2sum());

	// Evaluate and output measures
	const bool  evalf1 = args_info.f1f1_flag || args_info.f1pp_flag;
	if(evalf1)
		printf("F1_gm (%s average of %s): %G", args_info.unweighted_flag ? "unweighted" : "weighed"
			, args_info.f1pp_flag ? "partial probabilities" : "F1s"
			, Collection::f1gm(cn1, cn2, !args_info.unweighted_flag, args_info.f1pp_flag));

	if(args_info.nmi_flag) {
		if(evalf1)
			fputs(", ", stdout);
		printf("NMI: %G\n", Collection::nmi(cn1, cn2, args_info.ln_flag));
	} else puts("");  // \n

    return 0;
}
