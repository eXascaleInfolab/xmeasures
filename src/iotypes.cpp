//! \brief Shared File IO types.
//!
//!	Interface macro
//! INCLUDE_STL_FS  - include STL filesystem library under fs namespace. This macros is
//! 	defined to avoid repetitive conditional inclusion of the STL FS.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include <cstring>  // strlen
#include <cassert>
#include "iotypes.h"


bool StringBuffer::readline(FILE* input)
{
#if HEAVY_VALIDATION >= 2
	assert(input && "readline(), valid file stream should be specified");
#endif // HEAVY_VALIDATION
	// Read data from file until the string is read or an error occurs
	while(fgets(data() + m_cur, size() - m_cur, input) && data()[size()-2]) {
#if TRACE >= 3  // Verified
		fprintf(stderr, "readline(), resizing buffer of %lu bytes, %lu pos: %s\n"
			, size(), m_cur, data());
#endif // TRACE
		m_cur = size() - 1;  // Start overwriting ending '0' of the string
		resize(size() + (size() / (spagesize * 2) + 1) * spagesize);
		data()[size() - 2] = 0;  // Set prelast element to 0
	}
#if HEAVY_VALIDATION >= 2
	assert((!m_cur || strlen(data()) >= m_cur) && "readline(), string size validation failed");
#endif // HEAVY_VALIDATION
	m_cur = 0;  // Reset the writing (appending) position
	// Note: prelast and last elements of the buffer will be always zero

	// Check for errors
	if(feof(input) || ferror(input)) {
		if(ferror(input))
			perror("readline(), file reading error");
		return false;  // No more lines can be read
	}

	return true;  // More lines can be read
}
