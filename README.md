# xmeasures - Extrinsic Clustering Measures
Extremely fast evaluation of the extrinsic clustering measures: *various F1 measures (including F1-Score) for overlapping multi-resolution clusterings with unequal node base (and optional node base synchronization)* using various *matching policies (micro, macro and combined weighting)* and standard NMI for non-overlapping clustering on a single resolution. `xmeasures` also provides clusters labeling with the indices of the ground-truth clusters considering 1:n match and evaluating F1, precision and recall of the labeled clusters.  
`xmeasures` evaluates F1 and NMI for collections of hundreds thousands clusters withing a dozen seconds on an ordinary laptop using a single CPU core. The computational time is O(N) unlike O(N*C) of the existing state of the art implementations, where N is the number of nodes in the network and C is the number of clusters.
`xmeasures` is one of the utilities designed for the [PyCaBeM](https://github.com/eXascaleInfolab/PyCABeM) clustering benchmark to evaluate clustering of large networks.  
A paper about the implemented F1 measures (F1p is much more indicative and discriminative than the standard [Average F1-Score](https://cs.stanford.edu/people/jure/pubs/bigclam-wsdm13.pdf)), [NMI measures](www.jmlr.org/papers/volume11/vinh10a/vinh10a.pdf) and their applicability is being written now and the reference will be specified soon...
> Standard NMI is implemented considering overlapping and multi-resolution clustering only to demonstrate non-applicability of the standard NMI for such cases, where it yields unfair results. See [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) for the fair generalized NMI evaluation.

Author (c)  Artem Lutov <artem@exascale.info>

## Content
- [Deployment](#deployment)
	- [Requirements](#requirements)
	- [Compilation](#compilation)
- [Usage](#usage)
- [Related Projects](#related-projects)

# Deployment

The target platform is NIX, the binary is compiled for Linux Ubuntu x64 and also should work on Windows 10+ x64 (see details in [this article](https://www.howtogeek.com/249966/how-to-install-and-use-the-linux-bash-shell-on-windows-10/)).

## Requirements
There are no any requirements for the execution or compilation except the *standard C++ library*.

For the *prebuilt executable* on Linux Ubuntu 16.04 x64: `$ sudo apt-get install libstdc++6`.

## Compilation
```
$ make release
```
To update/extend the input parameters modify `args.ggo` and run `GenerateArgparser.sh` (calls `gengetopt`) before running `make`. To install [*gengetopt*](https://www.gnu.org/software/gengetopt) execute: `$ sudo apt-get install gengetopt`.

> Build errors might occur if the default *g++/gcc <= 5.x*.  
Then `g++-5` should be installed and `Makefile` might need to be edited replacing `g++`, `gcc` with `g++-5`, `gcc-5`.

# Usage
Execution Options:
```
$ ../xmeasures -h
xmeasures 3.2

Extrinsic measures evaluation: F1 (prob, harm and score) for overlapping
multi-resolution clusterings with possible unequal node base and standard NMI
for non-overlapping clustering on a single resolution.

Usage: xmeasures [OPTIONS] clustering1 clustering2

  clustering  - input file, collection of the clusters to be evaluated.
  
Examples:
  $ ./xmeasures -fp -kc networks/5K25.cnl tests/5K25_l0.825/5K25_l0.825_796.cnl
  $ ./xmeasures -fh -kc -i tests/5K5_l8.cll -ph -l gt/5K5.cnl tests/5K5_l8.cnl


Extrinsic measures are evaluated, i.e. clustering (collection of clusters) is
compared to another clustering, which can be a ground-truth. Optional labeling
of the evaluating clusters with the specified ground-truth clusters.
NOTE: Each cluster should contain unique members, which is verified only in the
debug mode.
Evaluating measures are:

  - F1  - various F1 measures of the Greatest (Max) Match including the Average
F1-Score with optional weighting.
 NOTE: There are 3 matching policies available for each kind of F1. The most
representative evaluation is performed by the F1p with combined matching
policy (considers both micro and macro weightings). 

  - NMI  - Normalized Mutual Information, normalized by either max or also
sqrt, avg and min information content denominators.
ATTENTION: This is standard NMI, which should be used ONLY for the HARD
partitioning evaluation (non-overlapping clustering on a single resolution).
It penalizes overlapping and multi-resolution structures.
NOTE: Unequal node base in the clusterings is allowed, it penalizes the
match.Use [OvpNMI](https://github.com/eXascaleInfolab/OvpNMI) or
[GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) for NMI evaluation
in the arbitrary collections (still each cluster should contain unique
members).


  -h, --help                    Print help and exit
  -V, --version                 Print version and exit
  -o, --ovp                     evaluate overlapping instead of
                                  multi-resolution clusters, where max matching
                                  for any shared member between R overlapping
                                  clusters is 1/R unlike 1 for the member
                                  existing in R distinct clusters on R
                                  resolutions  (default=off)
  -s, --sync=filename           synchronize with the node base, skipping the
                                  non-matching nodes.
                                  NOTE: The node base can be either a separate,
                                  or an evaluating CNL file, in the latter case
                                  this option should precede the evaluating
                                  filename not repeating it
  -m, --membership=FLOAT        average expected membership of the nodes in the
                                  clusters, > 0, typically >= 1. Used only for
                                  the containers preallocation facilitating
                                  estimation of the nodes number if not
                                  specified in the file header.  (default=`1')
  -d, --detailed                detailed (verbose) results output
                                  (default=off)

F1 Options:
  -f, --f1[=ENUM]               evaluate F1 of the [weighted] average of the
                                  greatest (maximal) match by F1 or partial
                                  probability.
                                  NOTE: F1p <= F1h <= F1s, where:
                                   - p (F1p)  - Harmonic mean of the [weighted]
                                  average of Partial Probabilities, the most
                                  indicative as satisfies the largest number of
                                  the Formal Constraints (homogeneity,
                                  completeness, rag bag, size/quantity,
                                  balance);
                                   - h (F1h)  - Harmonic mean of the [weighted]
                                  average of F1s;
                                   - s (F1s)  - Arithmetic mean (average) of
                                  the [weighted] average of F1s, Standard
                                  F1-Score, the least discriminative and
                                  satisfies the lowest number of the Formal
                                  Constraints.
                                    (possible values="partprob",
                                  "harmonic", "standard"
                                  default=`partprob')
  -k, --kind[=ENUM]             kind of the matching policy:
                                   - w  - Weighted by the number of nodes in
                                  each cluster
                                   - u  - Unweighed, where each cluster is
                                  treated equally
                                   - c  - Combined(w, u) using geometric mean
                                  (drops the value not so much as harmonic
                                  mean)
                                     (possible values="weighted",
                                  "unweighed", "combined"
                                  default=`weighted')

NMI Options:
  -n, --nmi                     evaluate NMI (Normalized Mutual Information)
                                  (default=off)
  -a, --all                     evaluate all NMIs using sqrt, avg and min
                                  denominators besides the max one
                                  (default=off)
  -e, --ln                      use ln (exp base) instead of log2 (Shannon
                                  entropy, bits) for the information measuring
                                  (default=off)

Clusters Labeling:
  -l, --label=gt_filename       label evaluating clusters with the specified
                                  ground-truth (gt) cluster indices and
                                  evaluate F1 (including Precision and Recall)
                                  of the MATCHED
                                   labeled clusters only (without the probable
                                  subclusters).
                                  NOTE: If 'sync' option is specified then the
                                  clusters labels file name should be the same
                                  as the node base (if specified) and should be
                                  in the .cnl format. The file name can be
                                  either a separate or an evaluating CNL file,
                                  in the latter case this option should precede
                                  the evaluating filename not repeating it
  -p, --policy[=ENUM]           Labels matching policy:
                                   - p  - Partial Probabilities (maximizes
                                  gain)
                                   - h  - Harmonic Mean (minimizes loss,
                                  maximizes F1)
                                    (possible values="partprob", "harmonic"
                                  default=`harmonic')
  -u, --unweighted              Labels weighting policy on F1 evaluation:
                                  weighted by the number of instances in each
                                  label or unweighed, where each label is
                                  treated equally  (default=off)
  -i, --identifiers=labels_filename
                                output labels (identifiers) of the evaluating
                                  clusters as lines of space-separated indices
                                  of the ground-truth clusters (.cll - clusters
                                  labels list)
                                  NOTE: If 'sync' option is specified then the
                                  reduce collection is outputted to the
                                  <labels_filename>.cnl besides the
                                  <labels_filename>
```

> Empty lines and comments (lines starting with #) in the input file (cnl format) are skipped.

**Examples**
Evaluate F1 of the weighted average of the greatest (maximal) match by partial probabilities (the most discriminative F1-measure) and using macro weighting (default as the most frequently used, thought combined weighting is the most indicative one):
```
$ ./xmeasures -f data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1 of the weighted average of the greatest (maximal) match by F1s:
```
$ ./xmeasures -fh data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1 of the [unweighted] average of the greatest (maximal) match by partial probabilities and synchronize the node base with the first evaluating collection, and considering overlapping clusters instead of multi-resolutions (`-o` does not matter for the case of non-overlapping single resolution collections):
```
$ ./xmeasures -sku -fp -o data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1-Score (weighted by the cluster size) and  NMI with all denominators synchronizing node base of the evaluating collections with `1lev4nds2cls.cnl`:
```
$ ./xmeasures -fs -na -s data/1lev4nds2cls.cnl data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate combined weighed and unweighted F1h (harmonic F1 average), label the clusters with the indices of provided labels, evaluate F1, precision and recall of the labeled clusters and output the labels to the `clslbs.cll`:
```
$ ./xmeasures -fh -kc -i clslbs.cll -l labels.cnl clusters.cnl
```


**Note:** Please, [star this project](https://github.com/eXascaleInfolab/xmeasures) if you use it.

# Related Projects
- [OvpNMI](https://github.com/eXascaleInfolab/OvpNMI)  - NMI evaluation for the overlapping clusters (communities) that is not compatible with the standard NMI value unlike GenConvNMI, but it is much faster than GenConvNMI.
- [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) - Overlapping NMI evaluation that is compatible with the original NMI and suitable for both overlapping and multi resolution (hierarchical) clustering evaluation.
- [resmerge](https://github.com/eXascaleInfolab/resmerge)  - Resolution levels clustering merger with filtering. Flattens hierarchy/list of multiple resolutions levels (clusterings) into the single flat clustering with clusters on various resolution levels synchronizing the node base.
- [ExecTime](https://bitbucket.org/lumais/exectime/)  - A lightweight resource consumption profiler.
- [PyCABeM](https://github.com/eXascaleInfolab/PyCABeM) - Python Benchmarking Framework for the Clustering Algorithms Evaluation. Uses extrinsic (NMIs) and intrinsic (Q) measures for the clusters quality evaluation considering overlaps (nodes membership by multiple clusters).
- [TInfES](https://github.com/eXascaleInfolab/TInfES)  - Type inference evaluation scripts and accessory apps used for the benchmarking.
