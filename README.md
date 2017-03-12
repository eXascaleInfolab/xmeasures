# xmeasures - Extrinsic Clustering Measures
Extremely fast evaluation of the extrinsic clustering measures: *various F1 measures (including F1-Score) for overlapping multi-resolution clusterings with unequal node base (and optional node base synchronization)* and standard NMI for non-overlapping clustering on a single resolution.  
`xmeasures` evaluates F1 and NMI for collections of hundreds thousands clusters withing a dozen seconds on an ordinary laptop using a single CPU core. The computational time is O(N) unlike O(N*C) of the existing state of the art implementations, where N is the number of nodes in the network and C is the number of clusters.
`xmeasures` is one of the utilities designed for the [PyCaBeM](https://github.com/eXascaleInfolab/PyCABeM) clustering benchmark to evaluate clustering of large networks.  
A paper about the implemented F1 (F1p is much more discriminative than the standard F1-Score and NF1 measure) and NMI measures and their applicability is being written now and the reference will be specified before Summer, 2017.
> Standard NMI is implemented considering overlapping and multi-resolution clustering only to demonstrate non-applicability of the standard NMI for such cases, it yield unfair results. See [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) for the fair generalized NMI evaluation.

Author (c)  Artem Lutov <artem@exascale.info>

## Content
- [Deployment](#deployment)
	- [Dependencies](#dependencies)
	- [Compilation](#compilation)
- [Usage](#usage)
- [Related Projects](#related-projects)

# Deployment

## Dependencies
There no any dependencies for the execution or compilation except the standard C++ library.  
For the *prebuilt executables*:
- libstdc++6 (stdc++fs): `$ sudo apt-get install libstdc++6`

## Compilation
Just execute `$ make`.  
To update/extend the input parameters modify `args.ggo` and run `GenerateArgparser.sh` (calls `gengetopt`) before running `make`. To install [*gengetopt*](https://www.gnu.org/software/gengetopt) execute: `$ sudo apt-get install gengetopt`.

# Usage
Execution Options:
```
$ ../xmeasures -h
xmeasures 3.0

Extrinsic measures evaluation: F1 (prob, harm and score) for overlapping
multi-resolution clusterings with possible unequal node base and standard NMI
for non-overlapping clustering on a single resolution.

Usage: xmeasures [OPTIONS] clustering1 clustering2

  clustering  - input file, collection of the clusters to be evaluated.

Extrinsic measures are evaluated, i.e. clustering (collection of clusters) is
compared to another collection, which is typically the ground-truth.
Evaluating measures are:
  - F1  - various F1 measures of the Greatest (Max) Match including the
standard F1-Score with optional weighting;
  - NMI  - Normalized Mutual Information, normalized by either max or also avg
and min information content denominators.
ATTENTION: this is standard NMI, which should be used ONLY for the HARD
partitioning evaluation (non-overlapping clustering on a single resolution).
it penalizes overlapping and multi-resolution structures.
NOTE: unequal node base in the clusterings is allowed, it penalizes the match.
Use [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) for arbitrary
collections evaluation.


  -h, --help              Print help and exit
  -V, --version           Print version and exit
  -o, --ovp               evaluate overlapping clusters instead of
                            multi-resolution  (default=off)
  -s, --sync=filename     synchronize with the node base, skipping the
                            non-matching nodes.
                            NOTE: the node base can be either a separate, or an
                            evaluating CNL file, in the latter case this option
                            should precede the evaluating filename not
                            repeating it
  -m, --membership=FLOAT  average expected membership of the nodes in the
                            clusters, > 0, typically >= 1. Used only for the
                            containers preallocation facilitating estimation of
                            the nodes number if not specified in the file
                            header.  (default=`1')
  -d, --detailed          detailed (verbose) results output  (default=off)

F1 Options:
  -f, --f1[=ENUM]         evaluate F1 of the [weighted] average of the greatest
                            (maximal) match by F1 or partial probability.
                            NOTE: F1p <= F1h <= F1s, where:
                             - F1p  - Harmonic mean of the [weighted] average
                            of partial probabilities, the most discriminative
                            and satisfies the largest number of the Formal
                            Constraints (homogeneity, completeness, rag bag,
                            size/quantity, balance);
                             - F1h  - Harmonic mean of the [weighted] average
                            of F1s;
                             - F1s  - Standard F1-Score, i.e. the Arithmetic
                            mean (average) of the [weighted] average of F1s,
                            the least discriminative and satisfies the lowest
                            number of the Formal Constraints.
                              (possible values="partprob", "harmonic",
                            "standard" default=`partprob')
  -u, --unweighted        evaluate simple average of the best matches instead
                            of weighted by the cluster size  (default=off)

NMI Options:
  -n, --nmi               evaluate NMI (Normalized Mutual Information)
                            (default=off)
  -a, --all               evaluate all NMIs using avg and min denominators
                            besides the max one  (default=off)
  -e, --ln                use ln (exp base) instead of log2 (Shannon entropy,
                            bits) for the information measuring  (default=off)
```

**Examples**
Evaluate F1 of the weighted average of the greatest (maximal) match by partial probabilities (the most discriminative F1-measure):
```
$ ./xmeasures -f data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1 of the weighted average of the greatest (maximal) match by F1s:
```
$ ./xmeasures -fh data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1 of the [unweighted] average of the greatest (maximal) match by partial probabilities and synchronize the node base with the first evaluating collection, and considering overlapping clusters instead of multi-resolutions (`-o` does not matter for the case of non-overlapping single resolution collections):
```
$ ./xmeasures -sufp -o data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1-Score (weighted by the cluster size) and  NMI with all denominators synchronizing node base of the evaluating collections with `1lev4nds2cls.cnl`:
```
$ ./xmeasures -fs -na -s data/1lev4nds2cls.cnl data/3cls5nds.cnl data/4cls6nds.cnl
```

# Related Projects
- [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) - Overlapping NMI evaluation that is compatible with the original NMI and suitable for both overlapping and multi resolution (hierarchical) clustering evaluation.
- [resmerge](https://github.com/eXascaleInfolab/resmerge)  - Resolution levels clustering merger with filtering. Flattens hierarchy/list of multiple resolutions levels (clusterings) into the single flat clustering with clusters on various resolution levels synchronizing the node base.
- [ExecTime](https://bitbucket.org/lumais/exectime/)  - A lightweight resource consumption profiler.
- [PyCABeM](https://github.com/eXascaleInfolab/PyCABeM) - Python Benchmarking Framework for the Clustering Algorithms Evaluation. Uses extrinsic (NMIs) and intrinsic (Q) measures for the clusters quality evaluation considering overlaps (nodes membership by multiple clusters).

**Note:** Please, [star this project](https://github.com/eXascaleInfolab/xmeasures) if you use it.
