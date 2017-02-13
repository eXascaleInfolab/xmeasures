//! \brief File IO utils.
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2017-02-13

#include <functional>  // hash
#include <cstring>  // strtok
#include <cmath>  // sqrt
#include <cassert>
#include <system_error>  // error_code

#ifdef __unix__
#include <sys/stat.h>
#endif // __unix__

#define INCLUDE_STL_FS
#include "fileio.h"


using std::string;
using std::error_code;
using std::to_string;
using fs::path;
using fs::create_directories;
using fs::is_directory;
using fs::exists;
using fs::status;

// Accessory types definitions -------------------------------------------------
void AggHash::add(Id id) noexcept
{
	++m_size;
	m_idsum += id;
	m_id2sum += id * id;
}

void AggHash::clear() noexcept
{
	m_size = 0;
	m_idsum = 0;
	m_id2sum = 0;
}

size_t AggHash::hash() const
{
	return std::hash<string>()(string(reinterpret_cast<const char*>(this), sizeof *this));
}

// File IO Types definitions ---------------------------------------------------
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
			perror("ERROR readline(), file reading error");
		return false;  // No more lines can be read
	}

	return true;  // More lines can be read
}

// File I/O functions ----------------------------------------------------------
void ensureDir(const string& dir)
{
#if TRACE >= 3
	fprintf(stderr, "ensureDir(), ensuring existence of: %s\n", dir.c_str());
#endif // TRACE
	// Check whether the output directory exists and create it otherwise
	path  outdir = dir;
	if(!exists(outdir)) {
		error_code  err;
		if(!create_directories(outdir, err))
			fputs(string("ERROR ensureDir(), target directory '").append(dir)
				.append("' can't be created: ").append(err.message())
				.append("\n").c_str(), stderr);
	} else if(!is_directory(outdir))
		fputs(string("ERROR ensureDir(), target entry '").append(dir)
			.append("' already exists as non-directory path\n").c_str(), stderr);
}

void parseHeader(NamedFileWrapper& fcls, StringBuffer& line, size_t& clsnum, size_t& ndsnum) {
    //! Parse id value
    //! \return  - id value of 0 in case of parsing errors
	auto parseId = []() -> Id {
		char* tok = strtok(nullptr, " \t,");  // Note: the value can't be ended with ':'
		//errno = 0;
		const auto val = strtoul(tok, nullptr, 10);
		if(errno)
			perror("WARNING parseId(), id value parsing error");
		return val;
	};

	// Process the header, which is a special initial comment
	// The target header is:  # Clusters: <cls_num>[,] Nodes: <cls_num>
	constexpr char  clsmark[] = "clusters";
	constexpr char  ndsmark[] = "nodes";
	constexpr char  attrnameDelim[] = " \t:,";
	while(line.readline(fcls)) {
		// Skip empty lines
		if(line.empty())
			continue;
		// Consider only subsequent comments
		if(line[0] != '#')
			break;

		// Tokenize the line
		char *tok = strtok(line + 1, attrnameDelim);  // Note: +1 to skip the leading '#'
		// Skip comment without the string continuation and continuous comment
		if(!tok || tok[0] == '#')
			continue;
		uint8_t  attrs = 0;  // The number of read attributes
		do {
			// Lowercase the token
			for(char* pos = tok; *pos; ++pos)
				*pos = tolower(*pos);

			// Identify the attribute and read it's value
			if(!strcmp(tok, clsmark)) {
				clsnum = parseId();
				++attrs;
			} else if(!strcmp(tok, ndsmark)) {
				ndsnum = parseId();
				++attrs;
			} else {
				fprintf(stderr, "WARNING parseHeader(), the header parsing is omitted"
					" because of the unexpected attribute: %s\n", tok);
				break;
			}
		} while((tok = strtok(nullptr, attrnameDelim)) && attrs < 2);

		// Validate and correct the number of clusters if required
		// Note: it's better to reallocate a container a few times than too much overconsume the memory
		if(ndsnum && clsnum > ndsnum) {
			fprintf(stderr, "WARNING parseHeader(), clsnum (%lu) typically should be"
				" less than ndsnum (%lu)\n", clsnum, ndsnum);
			clsnum = ndsnum;
			//assert(0 && "parseHeader(), clsnum typically should be less than ndsnum");
		}
		// Get following line for the unified subsequent processing
		line.readline(fcls);
		break;
	}
}

size_t fileSize(const NamedFileWrapper& file) noexcept
{
	size_t  cmsbytes = -1;
#ifdef __unix__  // sqrt(cmsbytes) lines => linebuf = max(4-8Kb, sqrt(cmsbytes) * 2) with dynamic realloc
	struct stat  filest;
	int fd = fileno(file);
	if(fd != -1 && !fstat(fd, &filest))
		return filest.st_size;
#endif // __unix
	error_code  err;
	cmsbytes = fs::file_size(file.name(), err);
	if(cmsbytes == size_t(-1))
		fprintf(stderr, "WARNING fileSize(), file size evaluation failed: %s\n"
			, err.message().c_str());

//	// Get length of the file
//	fseek(file, 0, SEEK_END);
//	cmsbytes = ftell(file);  // The number of bytes in the input communities
//	if(cmsbytes == size_t(-1))
//		perror("WARNING fileSize(), file size evaluation failed");
//	//fprintf(stderr, "  %s: %lu bytes\n", fname, cmsbytes);
//	rewind(file);  // Set position to the begin of the file

	return cmsbytes;
}

Id estimateNodes(size_t filesize, float membership) noexcept
{
	if(membership <= 0) {
		fprintf(stderr, "WARNING estimateNodes(), invalid membership = %G specified"
			", reseted to 1\n", membership);
		membership = 1;
		//throw invalid_argument("estimateNodes(), membership = "
		//	+ to_string(membership) + " should be positive\n");
	}

	Id  ndsnum = 0;  // The estimated number of nodes
	if(filesize) {
		size_t  magn = 10;  // Decimal ids magnitude
		unsigned  img = 1;  // Index of the magnitude (10^1)
		size_t  reminder = filesize % magn;  // Reminder in bytes
		ndsnum = reminder / ++img;  //  img digits + 1 delimiter for each element
		while(filesize >= magn) {
			magn *= 10;
			ndsnum += (filesize - reminder) % magn / ++img;
			reminder = filesize % magn;
		}
	}
	return ndsnum / membership;
}

Id estimateClusters(Id ndsnum, float membership) noexcept
{
	if(membership <= 0) {
		fprintf(stderr, "WARNING estimateClusters(), invalid membership = %G specified"
			", reseted to 1\n", membership);
		membership = 1;
		//throw invalid_argument("estimateClusters(), membership = "
		//	+ to_string(membership) + " should be positive\n");
	}

	Id  clsnum = 0;  // The estimated number of clusters
	// Usually the number of clusters does not increase square root of the number of nodes
	// Note: do not estimate in case the number of nodes is not specified
	if(ndsnum)
		clsnum = sqrt(ndsnum * membership) + 1;  // Note: +1 to consider rounding down
	return clsnum;
}
