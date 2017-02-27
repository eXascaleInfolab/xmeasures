# xmeasures - Extrinsic Clustering Measures
Extrinsic clustering measures evaluation for the multi-resolution clustering with overlaps (covers): F1_gm for overlapping multi-resolution clusterings with possible unequal node base and standard NMI for non-overlapping clustering on a single resolution.  
`xmeasures` is one of the utilities designed for the [PyCaBeM](https://github.com/eXascaleInfolab/PyCABeM) clustering benchmark.

## Content
- [Deployment](#deployment)
	- [Dependencies](#dependencies)
	- [Compilation](#compilation)
- [Usage](#usage)
- [Related Projects](#related-projects)

# Deployment

## Dependencies
There no any dependencies for the execution or compilation.  
However, to extend the input options and automatically regenerate the input parsing,
[*gengetopt*](https://www.gnu.org/software/gengetopt) application should be installed: `$ sudo apt-get install gengetopt`.  
For the *prebuilt executables*:
- libstdc++6 (stdc++fs): `$ sudo apt-get install libstdc++6`

## Compilation
Just execute `$ make`.  
To update/extend the input parameters modify `args.ggo` and run `GenerateArgparser.sh` (calls `gengetopt`).

# Usage
Execution Options:
```
$ ./xmeasures -h
xmeasures 2.3

Extrinsic measures evaluation: F1_gm for overlapping multi-resolution
clusterings with possible unequal node base and standard NMI for
non-overlapping clustering on a single resolution.

Usage: xmeasures [OPTIONS] clustering1 clustering2

  clustering  - input file, collection of the clusters to be evaluated.

Extrinsic measures are evaluated, i.e. clustering (collection of clusters) is
compared to another collection, which is typically the ground-truth.
Evaluating measures are:

  - F1_gm  - F1 of the [weighted] average greatest match evaluated by F1 or
partial probability

  - NMI  - Normalized Mutual Information, normalized by max or also avg and
mininformation content denominators.
ATTENTION: this is standard NMI, which should be used ONLY for the HARD
partitioning evaluation (non-overlapping clustering on a single resolution).
it penalized overlapping and multi-resolution structures.
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

F1 Options:
  -f, --f1                evaluate F1 of the [weighted] average of the greatest
                            (maximal) match by F1 or partial probability
                            (default=off)
  -p, --prob              use partial probability instead of the F1 for the
                            matching.
                            NOTE: typically F1pp < F1f1 and discriminates
                            similar collections better.  (default=off)
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
Evaluate F1 of the weighted average of the greatest (maximal) match by F1s:
```
$ ./xmeasures -f data/3cls5nds.cnl data/4cls6nds.cnl
```

Evaluate F1 of the [unweighted] average of the greatest (maximal) match by partial probabilities:
```
$ ./xmeasures -pu data/3cls5nds.cnl data/4cls6nds.cnl
```

# Related Projects
- [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) - Overlapping NMI evaluation that is compatible with the original NMI and suitable for both overlapping and multi resolution (hierarchical) clustering evaluation.
- [resmerge](https://github.com/eXascaleInfolab/resmerge)  - Resolution levels clustering merger with filtering. Flattens hierarchy/list of multiple resolutions levels (clusterings) into the single flat clustering with clusters on various resolution levels synchronizing the node base.
- [ExecTime](https://bitbucket.org/lumais/exectime/)  - A lightweight resource consumption profiler.
- [PyCABeM](https://github.com/eXascaleInfolab/PyCABeM) - Python Benchmarking Framework for the Clustering Algorithms Evaluation. Uses extrinsic (NMIs) and intrinsic (Q) measures for the clusters quality evaluation considering overlaps (nodes membership by multiple clusters).

**Note:** Please, [star this project](https://github.com/eXascaleInfolab/xmeasures) if you use it.
