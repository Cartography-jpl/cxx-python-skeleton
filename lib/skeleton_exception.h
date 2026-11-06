#ifndef SKELETON_EXCEPTION_H
#define SKELETON_EXCEPTION_H

#include "printable.h"
#include <sstream>		// Definition of ostringstream.

namespace Skeleton {

/****************************************************************//**
  This is the base of the exception hierarchy for Skeleton code. This
  can be written to like a stream to contain information about the
  exception. This is derived from the standard library std::exception
*******************************************************************/

class Exception: public std::exception, public Printable<Exception> {
public:
//-----------------------------------------------------------------------
/// Default constructor. Can give an optional string describing
/// the error.
//-----------------------------------------------------------------------

  Exception(const std::string& W = "") 
  { 
    s_ << W;  
  }

//-----------------------------------------------------------------------
/// Copy constructor.
//-----------------------------------------------------------------------

  Exception(const Exception& E)
  {
    try {
      // Stream the message in (rather than s_.str(...)), so the
      // stream is positioned at the end and a later << appends.
      s_ << E.s_.str();
    } catch(...) {		// Ignore all errors.
    }
  }
  
//-----------------------------------------------------------------------
/// Destructor.
//-----------------------------------------------------------------------
    
  ~Exception() noexcept override = default;

//-----------------------------------------------------------------------
/// Write to exception what() string.
//-----------------------------------------------------------------------
    
  template<class T> inline Exception& operator<<(const T& V)
  {
    s_ << V;
    return *this;
  }

//-----------------------------------------------------------------------
/// Print out description of object.
//-----------------------------------------------------------------------

  virtual void print(std::ostream& Os) const 
  {
    Os << "Skeleton Exception:\n"
       << "=========================\n" 
       << what() << "\n"
       << "=========================\n";
  }

//-----------------------------------------------------------------------
/// Description of what the error is.
//-----------------------------------------------------------------------

  const char* what() const noexcept override
  {
    scratch = s_.str();
    return scratch.c_str();
  }
private:
  mutable std::string scratch;
  std::ostringstream s_;
};

/** \defgroup Error Error checking routines */
/*@{*/
//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template <class T> inline void range_check_template(
const T&	   Val,		// Value to be checked.
const T&	   Min,		// Minimum allowed value.
const T&	   Max,		// Maximum allowed value.
const char*        File,
int                Line
)
{
  if(Val < Min ||
     !(Val < Max)) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n"
      << "Maximum allowed: " << Max;
    throw e;
  }
}

template <class T> inline void range_check_inclusive_template(
const T&	   Val,		// Value to be checked.
const T&	   Min,		// Minimum allowed value.
const T&	   Max,		// Maximum allowed value.
const char*        File,
int                Line
)
{
  if(Val < Min ||
     Val > Max) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n"
      << "Maximum allowed: " << Max;
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_check(V, Min, Max) \
      Skeleton::range_check_template(V, Min, Max, __FILE__, __LINE__)
#define range_check_inclusive(V, Min, Max) \
      Skeleton::range_check_inclusive_template(V, Min, Max, __FILE__, __LINE__)

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template <class T> inline void range_min_check_template(
const T&	   Val,		// Value to be checked.
const T&	   Min,		// Minimum allowed value.
const char*        File,
int                Line
)
{
  if(Val < Min) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n";
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_min_check(V, Min) \
      Skeleton::range_min_check_template(V, Min, __FILE__, __LINE__)

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template <class T> inline void range_max_check_template(
const T&	   Val,		// Value to be checked.
const T&	   Max,		// Maximum allowed value.
const char*        File,
int                Line
)
{
  if(!(Val < Max)) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Maximum allowed: " << Max << "\n";
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_max_check(V, Max) \
      Skeleton::range_max_check_template(V, Max, __FILE__, __LINE__)

}
/*@}*/
#endif
