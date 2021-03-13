#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
:Description: Python API and usage example for C API of the xmeasures library
Note that items in collections are allowed to be non-contiguous (i.e., hashes can be used as ids)

:Authors: (c) Artem Lutov <lua@lutan.ch>
:Date: 2020-03-12
"""

import pathlib
import numpy as np
from ctypes import Structure, CDLL, POINTER, c_uint, c_float #, c_void_p
from collections.abc import Iterable

# Python wrappers for C types ------------------------------------------------------------------------------------------
c_uint_p = POINTER(c_uint)
c_float_p = POINTER(c_float)
# null_ptr = c_void_p()


class ClusterNodes(Structure):
	_fields_ = [('num', c_uint),
				('ids', c_uint_p),
				('weights', c_float_p)]
ClusterNodesPtr = POINTER(ClusterNodes)

def clusterNodes(ids, weights=None):
	"""ClusterNodes initialization

	ids: iterable(uint)  - cluster node ids
	weights: iterable  - cluster node weights

	return ClusterNodes
	"""
	assert isinstance(ids, Iterable) and (weights is None or isinstance(weights, Iterable)), 'Invalid argument types'
	cnIds = (c_uint * len(ids))(*ids)
	cnWeights = c_float_p() if not weights else (c_float * len(weights))(*weights)
	return ClusterNodes(len(ids), cnIds, cnWeights)


class ClusterCollection(Structure):
	_fields_ = [('num', c_uint),
				('nodes', ClusterNodesPtr)]

def clusterCollection(clusters):
	"""ClusterCollection initialization

	clusters: iterable(iterable(uint))  - clusters (collection of nodes)

	return ClusterCollection
	"""
	assert isinstance(clusters, Iterable) and isinstance(clusters[0], Iterable), 'Invalid argument type'
	cc = (ClusterNodes * len(clusters))(*(clusterNodes(nds) for nds in clusters))
	return ClusterCollection(len(clusters), cc)

def weightedClusterCollection(clusters):
	"""ClusterCollection initialization

	nodes: iterable((iterable(uint), iterable(float)))  - weighted clusters (collections of nodes and their weights)

	return ClusterCollection
	"""
	assert isinstance(clusters, Iterable) and len(clusters[0]) == 2 and isinstance(clusters[0][0], Iterable), 'Invalid argument type'
	cc = (c_uint * len(clusters))(*(clusterNodes(nds, wgs) for nds, wgs in clusters))
	return ClusterCollection(len(clusters), cc)

# Example of xmeasures usage from Python -------------------------------------------------------------------------------
if __name__ == "__main__":
	# Load the shared library into ctypes
	libXms = pathlib.Path().absolute() / "bin/Release/libxmeasures.so"  # Release
	xms = CDLL(libXms)
	# Set proper return types for the importing functions
	xms.f1p.restype = c_float
	xms.f1h.restype = c_float
	xms.omegaExt.restype = c_float
	xms.omega.restype = c_float
	# Perform evaluations
	nc1 = clusterCollection(((9,2,4), (2,13)))
	nc2 = clusterCollection(((9,13,2), (2,4)))
	print('F1p: {}, F1h: {}, omegaExt: {}, omega: {}'.format(
		xms.f1p(nc1, nc2),
		xms.f1h(nc1, nc2),
		xms.omegaExt(nc1, nc2),
		xms.omega(nc1, nc2)
	))
