//! \brief File IO utils.
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

// Global MACROSES:
//	- HEAVY_VALIDATION  - use alternative evaluations to validate results
//		- 0  - turn off heavy validation
//		- 1  - default value for the heavy validation
//		- 2  - extra heavy validation (might duplicate already performed heavy validation)
//		- 3  - cross validation of functions (executed on each call, but only once is enough)
//
//	- TRACE, TRACE_EXTRA  - detailed tracing under debug (trace nodes weights)
//		- 0  - turn off the tracing
//		- 1  - brief tracing that can be used in release to show warnings, etc.
//		- 2  - detailed tracing for DEBUG
//		- 3  - extra detailed tracing
//
// NOTE: undefined maro definition is interpreted as having value 0

#ifndef TRACE
#ifdef DEBUG
	#define TRACE 2
#elif !defined(NDEBUG)  // RELEASE, !NDEBUG
	#define TRACE 1
//#else  // RELEASE, NDEBUG
//	#define TRACE 0
#endif // DEBUG
#endif // TRACE

#ifndef HEAVY_VALIDATION
#ifdef DEBUG
	#define HEAVY_VALIDATION 2
#elif !defined(NDEBUG)  // RELEASE, !NDEBUG
	#define HEAVY_VALIDATION 1
//#else  // ELEASE, NDEBUG
//	#define HEAVY_VALIDATION 0
#endif // DEBUG
#endif // HEAVY_VALIDATION


#ifndef FILEIO_H
#define FILEIO_H

#include <cstdint>
#include <cstdio>  // FILE
#include <utility>  // move
#include <string>
#include <vector>

#ifdef INCLUDE_STL_FS
#if defined(__has_include) && __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif defined(__has_include) && __has_include(<experimental/filesystem>)
	#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#else
	#error "STL filesystem is not available. The native alternative is not implemented."
#endif // __has_include
#endif // INCLUDE_STL_FS

//#include "types.h"


using std::move;
using std::string;
using std::vector;

// File Wrapping Types ---------------------------------------------------------
//! \brief Wrapper around the FILE* to prevent hanging file descriptors
class FileWrapper {
    FILE*  m_dsc;
    bool  m_tidy;
public:
    //! \brief Constructor
    //!
    //! \param fd FILE*  - the file descriptor to be held
    //! \param cleanup=true bool  - close the file descriptor on destruction
    //! 	(typically false if stdin/out is supplied)
    FileWrapper(FILE* fd=nullptr, bool cleanup=true) noexcept
    : m_dsc(fd), m_tidy(cleanup)  {}

    //! \brief Copy constructor
    //! \note Any file descriptor should have a single owner
    FileWrapper(const FileWrapper&)=delete;

	//! \brief Move constructor
	// ATTENTION: fw.m_dsc is not set to nullptr by the default move operation
	// ATTENTION: std::vector will move their elements if the elements' move constructor
	// is noexcept, and copy otherwise (unless the copy constructor is not accessible)
    FileWrapper(FileWrapper&& fw) noexcept
    : FileWrapper(fw.m_dsc, fw.m_tidy)
    {
    	fw.m_dsc = nullptr;
    }

    //! \brief Copy assignment
    //! \note Any file descriptor should have the single owner
    FileWrapper& operator= (const FileWrapper&)=delete;

	//! \brief Move assignment
	// ATTENTION: fw.m_dsc is not set to nullptr by the default move operation
    FileWrapper& operator= (FileWrapper&& fw) noexcept
    {
    	reset(fw.m_dsc, fw.m_tidy);
    	fw.m_dsc = nullptr;
    	return *this;
    }

    //! \brief Destructor
    ~FileWrapper()  // noexcept by default
    {
        if(m_dsc && m_tidy) {
            fclose(m_dsc);
            m_dsc = nullptr;
        }
    }

    //! \brief Implicit conversion to the file descriptor
    //!
    //! \return FILE*  - self as a file descriptor
    operator FILE*() const noexcept  { return m_dsc; }

    //! \brief Reset the wrapper
    //!
    //! \param fd FILE*  - the file descriptor to be held
    //! \param cleanup=true bool  - close the file descriptor on destruction
    //! 	(typically false if stdin/out is supplied)
    //! \return void
	void reset(FILE* fd=nullptr, bool cleanup=true) noexcept
	{
        if(m_dsc && m_tidy && m_dsc != fd)
            fclose(m_dsc);
    	m_dsc = fd;
    	m_tidy = cleanup;
	}

    //! \brief Release ownership of the holding file
    //!
    //! \return FILE*  - file descriptor
    FILE* release() noexcept
    {
    	auto fd = m_dsc;
    	m_dsc = nullptr;
		return fd;
    }
};

//! \brief Wrapper around the FILE* that holds also the filename giving ability
//! to reopen it and perform meaningful
// Note: we can't inherit from the FileWrapper because semantic of reset differs
class NamedFileWrapper {
	FileWrapper  m_file;  //!< File descriptor
	string  m_name;  //!< File name
public:
    //! \brief Constructor
    //! \pre Parent directory must exists
    //!
    //! \param filename const char*  - new file name to be opened
    //! \param mode const char*  - opening mode, the same as fopen() has
	NamedFileWrapper(const char* filename=nullptr, const char* mode=nullptr)
	: m_file(filename ? fopen(filename, mode) : nullptr)
	, m_name(filename ? filename : "")  {}

    //! \brief Copy constructor
    //! \note Any file descriptor should have a single owner
    NamedFileWrapper(const NamedFileWrapper&)=delete;

	//! \brief Move constructor
	// ATTENTION: std::vector will move their elements if the elements' move constructor
	// is noexcept, and copy otherwise (unless the copy constructor is not accessible)
    NamedFileWrapper(NamedFileWrapper&& fw) noexcept
    : m_file(move(fw.m_file)), m_name(move(fw.m_name))  {}

    //! \brief Copy assignment
    //! \note Any file descriptor should have the single owner
    NamedFileWrapper& operator= (const NamedFileWrapper&)=delete;

	//! \brief Move assignment
    NamedFileWrapper& operator= (NamedFileWrapper&& fw) noexcept
    {
    	m_file = move(fw.m_file);
    	m_name = move(fw.m_name);
    	return *this;
    }

    //! \brief File name
    //!
    //! \return const string&  - file name
    const string& name() const noexcept  { return m_name; }

    //! \brief Implicit conversion to the file descriptor
    //!
    //! \return FILE*  - file descriptor
    operator FILE*() const noexcept  { return m_file; }

    //! \brief Reopen the file under another mode
    //!
    //! \param mode const char*  - the mode of operations, the same as in fopen()
    //! \return NamedFileWrapper&  - the reopened file or closed (if can't be opened)
    NamedFileWrapper& reopen(const char* mode)
    {
		m_file.reset(freopen(nullptr, mode, m_file));  // m_name.c_str()
		return *this;
    }

    //! \brief Reset the file, closes current file and opens another one
    //! \pre Parent directory must exists
    //!
    //! \param filename const char*  - new file name to be opened
    //! \param mode const char*  - opening mode, the same as fopen() has
    //! \return NamedFileWrapper&  - the newly opened file or just the old one closed
	NamedFileWrapper& reset(const char* filename, const char* mode)
	{
		if(filename) {
			m_file.reset(fopen(filename, mode));
			m_name = filename;
		} else m_file.reset();
		return *this;
	}

    //! \brief Release ownership of the holding file
    //!
    //! \return FILE*  - file descriptor
    FILE* release() noexcept  { return m_file.release(); }
};

// File Reading Types ----------------------------------------------------------
//! \brief Base of the StringBuffer
using StringBufferBase = vector<char>;

//! \brief String buffer to real file by lines using c-strings
//! \note The last symbol in the string is always set to 0 automatically
class StringBuffer: protected StringBufferBase {
	constexpr static size_t  spagesize = 4096;  // Small page size on x64

	size_t  m_cur;  //! Current position for the writing
//protected:
//	StringBufferBase::size();
public:
    //! \brief
    //! \post the allocated buffer will have size >= 2
    //!
    //! \param size=spagesize size_t  - size of the buffer
	StringBuffer(size_t size=spagesize): StringBufferBase(size), m_cur(0)
	{
		if(size <= 2)
			size = 2;
		data()[0] = 0;  // Set first element to 0
		data()[size-2] = 0;  // Set prelast element to 0
		// Note: data()[size-1] is set to 0 automatically on file read if
		// the reading data size >= size - 1 bytes
	}

    //! \brief Reset the string and it's shrink the allocated buffer
    //!
    //! \param size=spagesize size_t  - new initial size of the string buffer
    //! \return void
	void reset(size_t size=spagesize)
	{
		// Reset writing position
		m_cur = 0;
		// Reset the buffer
		resize(size);
		shrink_to_fit();  // Free reserved memory
		data()[0] = 0;  // Set first element to 0
		data()[size-2] = 0;  // Set prelast element to 0
		// Note: data()[size-1] is set to 0 automatically on file read if
		// the reading data size >= size - 1 bytes
	}

    //! \brief Read line from the file and store including the terminating '\n' symbol
    //! \attention The read string contains the trailing '\n' if exist in the file
    //!
    //! \param input FILE*  - processing file
    //! \return bool  - whether the following line available and the current one
    //! 	is read without any errors
	bool readline(FILE* input);

    //! \brief whether the string is empty
    //!
    //! \return bool  - the line is empty
	bool empty() const  { return !front() || front() == '\n'; }

    //! \brief C-string including '\n' if it was present in the file
	operator char*() noexcept  { return data(); }

    //! \brief Const C-string including '\n' if it was present in the file
	operator const char*() const noexcept  { return data(); }

    //! \brief Make public indexing operators
	using StringBufferBase::operator[];
	using StringBufferBase::at;
};

// Base types declarations -----------------------------------------------------
//! Node Id
using Id = uint32_t;
//constexpr Id  ID_NONE = numeric_limits<Id>::max();

//! Size type
//! \note Larger than Id type with at least twice in magnitude
using Size = uint64_t;

// Accessory types -------------------------------------------------------------
//! \brief Aggregation hash of ids
class AggHash {
	Size  m_size;  //!< Size of the container
	Size  m_idsum;  //!<  Sum of the members
	Size  m_id2sum;  //!< Sum of the squared members
public:
    //! \brief Default constructor
	AggHash() noexcept
	: m_size(0), m_idsum(0), m_id2sum(0)  {}

    //! \brief Add id to the aggregation
    //!
    //! \param id Id  - id to be included into the hash
    //! \return void
	void add(Id id) noexcept;

    //! \brief Clear/reset the aggregation
    //!
    //! \return void
	void clear() noexcept;

    //! \brief Number of the aggregated ids
    //!
    //! \return size_t  - number of the aggregated ids
	size_t size() const noexcept  { return m_size; }

//    //! \brief The hash is empty
//    //!
//    //! \return bool  - the hash is empty
//	bool empty() const noexcept  { return !m_size; }

    //! \brief Evaluate hash of the aggregation
    //!
    //! \return size_t  - resulting hash
	size_t hash() const;
};

// File I/O functions ----------------------------------------------------------
//! \brief Ensure existence of the specified directory
//!
//! \param dir const string&  - directory to be created if has not existed
//! \return void
void ensureDir(const string& dir);

//! \brief Get file size
//!
//! \param file const NamedFileWrapper&  - target file
//! \return size_t  - file size, -1 on error
size_t fileSize(const NamedFileWrapper& file) noexcept;

//! \brief  Parse the header of CNL file and validate the results
//! \post clsnum <= ndsnum if ndsnum > 0. 0 means not specified
//!
//! \param fcls NamedFileWrapper&  - the reading file
//! \param line StringBuffer&  - processing line (string, header) being read from the file
//! \param[out] clsnum size_t&  - resulting number of clusters if specified, 0 in case of parsing errors
//! \param[out] ndsnum size_t&  - resulting number of nodes if specified, 0 in case of parsing errors
//! \return void
void parseHeader(NamedFileWrapper& fcls, StringBuffer& line, size_t& clsnum, size_t& ndsnum);

//! \brief Estimate the number of nodes from the CNL file size
//!
//! \param filesize size_t  - the number of bytes in the CNL file
//! \param membership=1.f float  - average membership of the node,
//! 	> 0, typically ~= 1
//! \return Id  - estimated number of nodes
Id estimateNodes(size_t filesize, float membership=1.f) noexcept;

//! \brief Estimate the number of clusters from the number of nodes
//!
//! \param ndsnum Id - the number of nodes
//! \param membership=1.f float  - average membership of the node,
//! 	> 0, typically ~= 1
//! \return Id  - estimated number of clusters
Id estimateClusters(Id ndsnum, float membership=1.f) noexcept;

#endif // FILEIO_H
