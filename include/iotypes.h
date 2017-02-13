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

#ifndef IOTYPES_H
#define IOTYPES_H

#include <cstdio>  // FILE
#include <utility>  // move
#include <string>


using std::move;
using std::string;

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

#endif // IOTYPES_H
