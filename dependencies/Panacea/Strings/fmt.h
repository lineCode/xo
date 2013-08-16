#pragma once

// If you don't want to use XStringA, then you must define FMT_STRING to be some kind of string class
// that can construct itself from const char*.

// To use std::string, do the following before including fmt.h:
// #define FMT_STRING          std::string
// #define FMT_STRING_BUF(s)   (s).c_str()
// #define FMT_STRING_LEN(s)   (s).Length()

#ifndef FMT_STRING
#include "XString.h"
#define FMT_STRING			XStringA
#define FMT_STRING_BUF(s)	static_cast<const char*>(s)
#define FMT_STRING_LEN(s)	(s).Length()
#endif


/*

fmt: (Yet another) typesafe, cross-platform (Windows,linux) printf replacement.

This thing uses snprintf as a backend, so all of the regular formatting commands
that you expect from the printf family of functions works.

Usage:
fmt( "%v %v", "abc", 123 ) --> "abc 123"				<== Use %v as a generic value type
fmt( "%s %d", "abc", 123 ) --> "abc 123"				<== Specific value types are fine too, unless they conflict with the provided type, in which case they are overridden
fmt( "%v", std::string("abc") ) --> "abc"				<== std::string
fmt( "%v", std::wstring("abc") ) --> "abc"				<== std::wstring
fmt( "%v", XStringA("abc") ) --> "abc"					<== XStringA
fmt( "%v", XStringW("abc") ) --> "abc"					<== XStringW
fmt( "%.3f", 25.5 ) --> "25.500"						<== Use format strings as usual

Known unsupported features:
* Positional arguments
* %*s (integer width parameter)	-- wouldn't be hard to add. Presently ignored.

Exclusively returns FMT_STRING.

By providing a cast operator to fmtarg, you can get an arbitrary type supported as an argument (provided it fits into one of the moulds of the printf family of arguments).

*/

class PAPI fmtarg
{
public:
	enum Types
	{
		TCStr,
		TWStr,
		TI32,
		TU32,
		TI64,
		TU64,
		TDbl,
	};
	union
	{
		const char*		CStr;
		const wchar_t*	WStr;
		int32			I32;
		int32			UI32;
		int64			I64;
		int64			UI64;
		double			Dbl;
	};
	Types Type;

	fmtarg( const char* v )					: Type(TCStr), CStr(v) {}
	fmtarg( const wchar_t* v )				: Type(TWStr), WStr(v) {}
#ifdef XSTRING_DEFINED
	fmtarg( const XStringT<char>& v )		: Type(TCStr), CStr((const char*) v) {}
	fmtarg( const XStringT<wchar_t>& v )	: Type(TWStr), WStr((const wchar_t*) v) {}
#endif
	fmtarg( const std::string& v )			: Type(TCStr), CStr(v.c_str()) {}
	fmtarg( const std::wstring& v )			: Type(TWStr), WStr(v.c_str()) {}
	fmtarg( const int32& v )				: Type(TI32), I32(v) {}
	fmtarg( const uint32& v )				: Type(TU32), UI32(v) {}
#ifdef _WIN32
	fmtarg( const long& v )					: Type(TI32), I32(v) {}
	fmtarg( const unsigned long& v )		: Type(TU32), UI32(v) {}
#endif
	fmtarg( const int64& v )				: Type(TI64), I64(v) {}
	fmtarg( const uint64& v )				: Type(TU64), UI64(v) {}
	fmtarg( const double& v )				: Type(TDbl), Dbl(v) {}

};

PAPI FMT_STRING fmt( const char* fs );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14, const fmtarg& a15 );
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14, const fmtarg& a15, const fmtarg& a16 );

// Clang will need -Wno-variadic-macros
// I believe the use of __VA_ARGS__ is cross-platform enough for our needs
#define fmtout( fs, ... )			fmt_write(stdout, fmt(fs, __VA_ARGS__))
#define fmtoutf( file, fs, ... )	fmt_write(file, fmt(fs, __VA_ARGS__))

// We cannot do fmtout and fmtoutf with macros, because you cannot use __VA_ARGS__ inside a nested macro.
// It's hard to explain in a few short words, but this was the old version, an it's pre-processed output
// #define fmtout( fs, ... )           fputs(FMT_STRING_TO_C(fmt(fs, __VA_ARGS__)), stdout)		--> fputs(static_cast<const char*>(fmt("Hello!", )), (&__iob_func()[1]));
// Notice the trailing comma. That trailing comma is NOT present if you don't have the FMT_STRING_TO_C there.

// The approach of making this proper function has the added advantage that we can write strings with null characters in them
// Returns the number of characters written (ie the result of fwrite).
PAPI size_t fmt_write( FILE* file, const FMT_STRING& s );

/*  cross-platform "snprintf"

	destination			Destination buffer
	count				Number of characters available in 'destination'. This must include space for the null terminating character.
	format_str			The format string
	return
		-1				Not enough space
		0..count-1		Number of characters written, excluding the null terminator. The null terminator was written though.
*/
PAPI int fmt_snprintf( char* destination, size_t count, const char* format_str, ... );

// Identical in all respects to fmt_snprintf, except that we deal with wide character strings
PAPI int fmt_swprintf( wchar_t* destination, size_t count, const wchar_t* format_str, ... );
