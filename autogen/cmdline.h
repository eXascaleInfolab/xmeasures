/** @file cmdline.h
 *  @brief The header file for the command line option parser
 *  generated by GNU Gengetopt version 2.22.6
 *  http://www.gnu.org/software/gengetopt.
 *  DO NOT modify this file, since it can be overwritten
 *  @author GNU Gengetopt by Lorenzo Bettini */

#ifndef CMDLINE_H
#define CMDLINE_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h> /* for FILE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
/** @brief the program name (used for printing errors) */
#define CMDLINE_PARSER_PACKAGE "xmeasures"
#endif

#ifndef CMDLINE_PARSER_PACKAGE_NAME
/** @brief the complete program name (used for help and version) */
#define CMDLINE_PARSER_PACKAGE_NAME "xmeasures"
#endif

#ifndef CMDLINE_PARSER_VERSION
/** @brief the program version */
#define CMDLINE_PARSER_VERSION "4.0"
#endif

enum enum_f1 { f1__NULL = -1, f1_arg_partprob = 0, f1_arg_harmonic, f1_arg_average };
enum enum_kind { kind__NULL = -1, kind_arg_weighted = 0, kind_arg_unweighed, kind_arg_combined };
enum enum_policy { policy__NULL = -1, policy_arg_partprob = 0, policy_arg_harmonic };

/** @brief Where the command line options are stored */
struct gengetopt_args_info
{
  const char *help_help; /**< @brief Print help and exit help description.  */
  const char *version_help; /**< @brief Print version and exit help description.  */
  int ovp_flag;	/**< @brief evaluate overlapping instead of multi-resolution clusters, where max matching for any shared member between R overlapping clusters is 1/R instead of 1 for the member belonging to R distinct clusters on R resolutions.
  NOTE: It has no effect for the Omega Index evaluation. (default=off).  */
  const char *ovp_help; /**< @brief evaluate overlapping instead of multi-resolution clusters, where max matching for any shared member between R overlapping clusters is 1/R instead of 1 for the member belonging to R distinct clusters on R resolutions.
  NOTE: It has no effect for the Omega Index evaluation. help description.  */
  int unique_flag;	/**< @brief ensure on loading that all cluster members are unique by removing the duplicates. (default=off).  */
  const char *unique_help; /**< @brief ensure on loading that all cluster members are unique by removing the duplicates. help description.  */
  char * sync_arg;	/**< @brief synchronize with the node base, skipping the non-matching nodes.
  NOTE: The node base can be either a separate, or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it.  */
  char * sync_orig;	/**< @brief synchronize with the node base, skipping the non-matching nodes.
  NOTE: The node base can be either a separate, or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it original value given at command line.  */
  const char *sync_help; /**< @brief synchronize with the node base, skipping the non-matching nodes.
  NOTE: The node base can be either a separate, or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it help description.  */
  float membership_arg;	/**< @brief average expected membership of the nodes in the clusters, > 0, typically >= 1. Used only to facilitate estimation of the nodes number on the containers preallocation if this number is not specified in the file header. (default='1').  */
  char * membership_orig;	/**< @brief average expected membership of the nodes in the clusters, > 0, typically >= 1. Used only to facilitate estimation of the nodes number on the containers preallocation if this number is not specified in the file header. original value given at command line.  */
  const char *membership_help; /**< @brief average expected membership of the nodes in the clusters, > 0, typically >= 1. Used only to facilitate estimation of the nodes number on the containers preallocation if this number is not specified in the file header. help description.  */
  int detailed_flag;	/**< @brief detailed (verbose) results output (default=off).  */
  const char *detailed_help; /**< @brief detailed (verbose) results output help description.  */
  int omega_flag;	/**< @brief evaluate Omega Index (a fuzzy version of the Adjusted Rand Index, identical to  the Fuzzy Rand Index). (default=off).  */
  const char *omega_help; /**< @brief evaluate Omega Index (a fuzzy version of the Adjusted Rand Index, identical to  the Fuzzy Rand Index). help description.  */
  int extended_flag;	/**< @brief evaluate extended Omega Index, which does not excessively penalize distinct node shares. (default=off).  */
  const char *extended_help; /**< @brief evaluate extended Omega Index, which does not excessively penalize distinct node shares. help description.  */
  enum enum_f1 f1_arg;	/**< @brief evaluate F1 of the [weighted] average of the greatest (maximal) match by F1 or partial probability.
  NOTE: F1p <= F1h <= F1s, where:
   - p (F1p)  - Harmonic mean of the [weighted] average of Partial Probabilities, the most indicative as satisfies the largest number of the Formal Constraints (homogeneity, completeness, rag bag, size/quantity, balance);
   - h (F1h)  - Harmonic mean of the [weighted] average of F1s;
   - a (F1a)  - Arithmetic mean (average) of the [weighted] average of F1s, the least discriminative and satisfies the lowest number of the Formal Constraints.
 (default='partprob').  */
  char * f1_orig;	/**< @brief evaluate F1 of the [weighted] average of the greatest (maximal) match by F1 or partial probability.
  NOTE: F1p <= F1h <= F1s, where:
   - p (F1p)  - Harmonic mean of the [weighted] average of Partial Probabilities, the most indicative as satisfies the largest number of the Formal Constraints (homogeneity, completeness, rag bag, size/quantity, balance);
   - h (F1h)  - Harmonic mean of the [weighted] average of F1s;
   - a (F1a)  - Arithmetic mean (average) of the [weighted] average of F1s, the least discriminative and satisfies the lowest number of the Formal Constraints.
 original value given at command line.  */
  const char *f1_help; /**< @brief evaluate F1 of the [weighted] average of the greatest (maximal) match by F1 or partial probability.
  NOTE: F1p <= F1h <= F1s, where:
   - p (F1p)  - Harmonic mean of the [weighted] average of Partial Probabilities, the most indicative as satisfies the largest number of the Formal Constraints (homogeneity, completeness, rag bag, size/quantity, balance);
   - h (F1h)  - Harmonic mean of the [weighted] average of F1s;
   - a (F1a)  - Arithmetic mean (average) of the [weighted] average of F1s, the least discriminative and satisfies the lowest number of the Formal Constraints.
 help description.  */
  enum enum_kind kind_arg;	/**< @brief kind of the matching policy:
   - w  - Weighted by the number of nodes in each cluster
   - u  - Unweighed, where each cluster is treated equally
   - c  - Combined(w, u) using geometric mean (drops the value not so much as harmonic mean)
 (default='weighted').  */
  char * kind_orig;	/**< @brief kind of the matching policy:
   - w  - Weighted by the number of nodes in each cluster
   - u  - Unweighed, where each cluster is treated equally
   - c  - Combined(w, u) using geometric mean (drops the value not so much as harmonic mean)
 original value given at command line.  */
  const char *kind_help; /**< @brief kind of the matching policy:
   - w  - Weighted by the number of nodes in each cluster
   - u  - Unweighed, where each cluster is treated equally
   - c  - Combined(w, u) using geometric mean (drops the value not so much as harmonic mean)
 help description.  */
  char * label_arg;	/**< @brief label evaluating clusters with the specified ground-truth (gt) cluster indices and evaluate F1 (including Precision and Recall) of the MATCHED labeled clusters only (without the probable subclusters).
  NOTE: If 'sync' option is specified then the clusters labels file name should be the same as the node base (if specified) and should be in the .cnl format. The file name can be either a separate or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it.  */
  char * label_orig;	/**< @brief label evaluating clusters with the specified ground-truth (gt) cluster indices and evaluate F1 (including Precision and Recall) of the MATCHED labeled clusters only (without the probable subclusters).
  NOTE: If 'sync' option is specified then the clusters labels file name should be the same as the node base (if specified) and should be in the .cnl format. The file name can be either a separate or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it original value given at command line.  */
  const char *label_help; /**< @brief label evaluating clusters with the specified ground-truth (gt) cluster indices and evaluate F1 (including Precision and Recall) of the MATCHED labeled clusters only (without the probable subclusters).
  NOTE: If 'sync' option is specified then the clusters labels file name should be the same as the node base (if specified) and should be in the .cnl format. The file name can be either a separate or an evaluating CNL file, in the latter case this option should precede the evaluating filename not repeating it help description.  */
  enum enum_policy policy_arg;	/**< @brief Labels matching policy:
   - p  - Partial Probabilities (maximizes gain)
   - h  - Harmonic Mean (minimizes loss, maximizes F1)
 (default='harmonic').  */
  char * policy_orig;	/**< @brief Labels matching policy:
   - p  - Partial Probabilities (maximizes gain)
   - h  - Harmonic Mean (minimizes loss, maximizes F1)
 original value given at command line.  */
  const char *policy_help; /**< @brief Labels matching policy:
   - p  - Partial Probabilities (maximizes gain)
   - h  - Harmonic Mean (minimizes loss, maximizes F1)
 help description.  */
  int unweighted_flag;	/**< @brief Labels weighting policy on F1 evaluation: weighted by the number of instances in each label or unweighed, where each label is treated equally (default=off).  */
  const char *unweighted_help; /**< @brief Labels weighting policy on F1 evaluation: weighted by the number of instances in each label or unweighed, where each label is treated equally help description.  */
  char * identifiers_arg;	/**< @brief output labels (identifiers) of the evaluating clusters as lines of space-separated indices of the ground-truth clusters (.cll - clusters labels list)
  NOTE: If 'sync' option is specified then the reduce collection is outputted to the <labels_filename>.cnl besides the <labels_filename>
.  */
  char * identifiers_orig;	/**< @brief output labels (identifiers) of the evaluating clusters as lines of space-separated indices of the ground-truth clusters (.cll - clusters labels list)
  NOTE: If 'sync' option is specified then the reduce collection is outputted to the <labels_filename>.cnl besides the <labels_filename>
 original value given at command line.  */
  const char *identifiers_help; /**< @brief output labels (identifiers) of the evaluating clusters as lines of space-separated indices of the ground-truth clusters (.cll - clusters labels list)
  NOTE: If 'sync' option is specified then the reduce collection is outputted to the <labels_filename>.cnl besides the <labels_filename>
 help description.  */
  int nmi_flag;	/**< @brief evaluate NMI (Normalized Mutual Information) (default=off).  */
  const char *nmi_help; /**< @brief evaluate NMI (Normalized Mutual Information) help description.  */
  int all_flag;	/**< @brief evaluate all NMIs using sqrt, avg and min denominators besides the max one (default=off).  */
  const char *all_help; /**< @brief evaluate all NMIs using sqrt, avg and min denominators besides the max one help description.  */
  int ln_flag;	/**< @brief use ln (exp base) instead of log2 (Shannon entropy, bits) for the information measuring (default=off).  */
  const char *ln_help; /**< @brief use ln (exp base) instead of log2 (Shannon entropy, bits) for the information measuring help description.  */
  
  unsigned int help_given ;	/**< @brief Whether help was given.  */
  unsigned int version_given ;	/**< @brief Whether version was given.  */
  unsigned int ovp_given ;	/**< @brief Whether ovp was given.  */
  unsigned int unique_given ;	/**< @brief Whether unique was given.  */
  unsigned int sync_given ;	/**< @brief Whether sync was given.  */
  unsigned int membership_given ;	/**< @brief Whether membership was given.  */
  unsigned int detailed_given ;	/**< @brief Whether detailed was given.  */
  unsigned int omega_given ;	/**< @brief Whether omega was given.  */
  unsigned int extended_given ;	/**< @brief Whether extended was given.  */
  unsigned int f1_given ;	/**< @brief Whether f1 was given.  */
  unsigned int kind_given ;	/**< @brief Whether kind was given.  */
  unsigned int label_given ;	/**< @brief Whether label was given.  */
  unsigned int policy_given ;	/**< @brief Whether policy was given.  */
  unsigned int unweighted_given ;	/**< @brief Whether unweighted was given.  */
  unsigned int identifiers_given ;	/**< @brief Whether identifiers was given.  */
  unsigned int nmi_given ;	/**< @brief Whether nmi was given.  */
  unsigned int all_given ;	/**< @brief Whether all was given.  */
  unsigned int ln_given ;	/**< @brief Whether ln was given.  */

  char **inputs ; /**< @brief unamed options (options without names) */
  unsigned inputs_num ; /**< @brief unamed options number */
} ;

/** @brief The additional parameters to pass to parser functions */
struct cmdline_parser_params
{
  int override; /**< @brief whether to override possibly already present options (default 0) */
  int initialize; /**< @brief whether to initialize the option structure gengetopt_args_info (default 1) */
  int check_required; /**< @brief whether to check that all required options were provided (default 1) */
  int check_ambiguity; /**< @brief whether to check for options already specified in the option structure gengetopt_args_info (default 0) */
  int print_errors; /**< @brief whether getopt_long should print an error message for a bad option (default 1) */
} ;

/** @brief the purpose string of the program */
extern const char *gengetopt_args_info_purpose;
/** @brief the usage string of the program */
extern const char *gengetopt_args_info_usage;
/** @brief the description string of the program */
extern const char *gengetopt_args_info_description;
/** @brief all the lines making the help output */
extern const char *gengetopt_args_info_help[];

/**
 * The command line parser
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser (int argc, char **argv,
  struct gengetopt_args_info *args_info);

/**
 * The command line parser (version with additional parameters - deprecated)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param override whether to override possibly already present options
 * @param initialize whether to initialize the option structure my_args_info
 * @param check_required whether to check that all required options were provided
 * @return 0 if everything went fine, NON 0 if an error took place
 * @deprecated use cmdline_parser_ext() instead
 */
int cmdline_parser2 (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);

/**
 * The command line parser (version with additional parameters)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param params additional parameters for the parser
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_ext (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  struct cmdline_parser_params *params);

/**
 * Save the contents of the option struct into an already open FILE stream.
 * @param outfile the stream where to dump options
 * @param args_info the option struct to dump
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_dump(FILE *outfile,
  struct gengetopt_args_info *args_info);

/**
 * Save the contents of the option struct into a (text) file.
 * This file can be read by the config file parser (if generated by gengetopt)
 * @param filename the file where to save
 * @param args_info the option struct to save
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_file_save(const char *filename,
  struct gengetopt_args_info *args_info);

/**
 * Print the help
 */
void cmdline_parser_print_help(void);
/**
 * Print the version
 */
void cmdline_parser_print_version(void);

/**
 * Initializes all the fields a cmdline_parser_params structure 
 * to their default values
 * @param params the structure to initialize
 */
void cmdline_parser_params_init(struct cmdline_parser_params *params);

/**
 * Allocates dynamically a cmdline_parser_params structure and initializes
 * all its fields to their default values
 * @return the created and initialized cmdline_parser_params structure
 */
struct cmdline_parser_params *cmdline_parser_params_create(void);

/**
 * Initializes the passed gengetopt_args_info structure's fields
 * (also set default values for options that have a default)
 * @param args_info the structure to initialize
 */
void cmdline_parser_init (struct gengetopt_args_info *args_info);
/**
 * Deallocates the string fields of the gengetopt_args_info structure
 * (but does not deallocate the structure itself)
 * @param args_info the structure to deallocate
 */
void cmdline_parser_free (struct gengetopt_args_info *args_info);

/**
 * Checks that all the required options were specified
 * @param args_info the structure to check
 * @param prog_name the name of the program that will be used to print
 *   possible errors
 * @return
 */
int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);

extern const char *cmdline_parser_f1_values[];  /**< @brief Possible values for f1. */
extern const char *cmdline_parser_kind_values[];  /**< @brief Possible values for kind. */
extern const char *cmdline_parser_policy_values[];  /**< @brief Possible values for policy. */


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CMDLINE_H */
