//! \brief Extrinsic measures evaluation interface.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include "interface.h"

Clusters loadClustering(const char* filename, float membership)
{
	Clusters  cls;

	return cls;
}

float evalF1(const Clusters& cls1, const Clusters& cls2)
{
	return 0;
}

float evalNmi(const Clusters& cls1, const Clusters& cls2)
{
	return 0;
}
