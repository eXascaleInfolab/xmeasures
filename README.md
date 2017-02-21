# xmeasures - Extrinsic Clustering Measures
Extrinsic clustering measures evaluation for the multi-resolution clustering with overlaps (covers): F1_gwah and NMI_om.  
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
xmeasures 2.0

Extrinsic measures evaluation for overlapping multiresolution clusterings with
possible inequal node base.

Usage: xmeasures [OPTIONS] clustering1 clustering2

  clustering  - input file, collection of the clusters to be evaluated

  -h, --help              Print help and exit
  -V, --version           Print version and exit
  -m, --membership=FLOAT  average expected membership of the nodes in the
                            clusters, > 0, typically >= 1  (default=`1')

 Mode: f1
  Evaluation of the F1 Max Average Harmonic Mean
  -f, --f1                evaluate F1 Max Average Harmonic Mean  (default=off)
  -w, --weighted          evaluate weighted average by cluster size
                            (default=off)

 Mode: nmi
  Evaluation of the Normalized Mutual Information
  -n, --nmi               evaluate NMI  (default=off)
  -e, --ln                use ln (exp base) instead of log2 (Shannon entropy,
                            bits) for the information measuring  (default=off)
```

**Examples**
Evaluate F1_gwah (maximal [greatest] weighted average harmonic mean):
```
$ ./xmeasures -fw data/3cls5nds.cln data/4cls6nds.cln
```

# Related Projects
- [GenConvNMI](https://github.com/eXascaleInfolab/GenConvNMI) - Overlapping NMI evaluation that is compatible with the original NMI and suitable for both overlapping and multi resolution (hierarchical) clustering evaluation.
- [resmerge](https://github.com/eXascaleInfolab/resmerge)  - Resolution levels clustering merger with filtering. Flattens hierarchy/list of multiple resolutions levels (clusterings) into the single flat clustering with clusters on various resolution levels synchronizing the node base.
- [ExecTime](https://bitbucket.org/lumais/exectime/)  - A lightweight resource consumption profiler.
- [PyCABeM](https://github.com/eXascaleInfolab/PyCABeM) - Python Benchmarking Framework for the Clustering Algorithms Evaluation. Uses extrinsic (NMIs) and intrinsic (Q) measures for the clusters quality evaluation considering overlaps (nodes membership by multiple clusters).

**Note:** Please, [star this project](https://github.com/eXascaleInfolab/xmeasures) if you use it.
