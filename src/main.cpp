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

#include "cmdline.h"  // Arguments parsing

int main(int argc, char **argv)
{
	gengetopt_args_info  args_info;
	auto  err = cmdline_parser(argc, argv, &args_info);
	if(err)
		return err;

	// Validate arguments
	if(!args_info.f1_given && !args_info.nmi_given) {
		fputs("WARNING, no any measures to evaluate specified\n", stderr);
		return 1;
	}

	if(args_info.inputs_num != 2) {
		fputs("ERROR, 2 input clusterings are required\n", stderr);
		cmdline_parser_print_help();
		return 1;
	}

	if(args_info.membership_arg <= 0) {
		fprintf(stderrm "ERROR, positive membership is expected: %G\n", args_info.membership_arg);
		return 1;
	}

	// Load collections as relations
	auto rels1 = loadClustering(args_info.inputs[0], args_info.membership_arg);
	auto rels2 = loadClustering(args_info.inputs[1], args_info.membership_arg);

	// Evaluate and output measures
	if(args_info.f1_flag)
		printf("F1_MAH: %G", evalF1(rels1, rels2));

	if(args_info.nmi_flag) {
		if(args_info.f1_flag)
			fputs(", ", stdout);
		printf("NMI: %G", evalNmi(rels1, rels2));
	}
	puts("");  // \n

    return 0;
}
