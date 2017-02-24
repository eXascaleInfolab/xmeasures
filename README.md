# xmeasures - Extrinsic Clustering Measures
Extrinsic clustering measures evaluation for the multi-resolution clustering with overlaps (covers): F1_gm and NMI (compatible to the standard NMI when applied to the single resolution non-overlapping collections of clusters).  
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
$ ./xmeasures  -h
xmeasures 2.1

Extrinsic measures evaluation for overlapping multi-resolution clusterings with
possible unequal node base: F1_gm and NMI.

Usage: xmeasures [OPTIONS] clustering1 clustering2

  clustering  - input file, collection of the clusters to be evaluated

  -h, --help              Print help and exit
  -V, --version           Print version and exit
  -m, --membership=FLOAT  average expected membership of the nodes in the
                            clusters, > 0, typically >= 1  (default=`1')

 Mode: f1
  F1 evaluation of the [weighted] average of the greatest (maximal) match by F1
  or partial probability.
   F1 evaluates clusters on multiple resolutions and applicable for overlapping
  clustering only as approximate evaluation
  -f, --f1f1              evaluate F1 of the [weighted] average of the greatest
                            (maximal) match by F1  (default=off)
  -p, --f1pp              evaluate F1 of the [weighted] average of the greatest
                            (maximal) match by partial probability.
                             NOTE: typically F1pp < F1f1 and fits to evaluate
                            similar collections  (default=off)
  -u, --unweighted        evaluate simple average of the best matches instead
                            of weighted by the cluster size  (default=off)

 Mode: nmi
  NMI (Normalized Mutual Information) evaluation.
  Standard NMI is evaluated, which is not applicable for overlapping or
  multi-resolution clustering
  -n, --nmi               evaluate NMI  (default=off)
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
