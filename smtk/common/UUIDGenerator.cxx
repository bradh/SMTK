//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "smtk/common/UUIDGenerator.h"

#include "smtk/common/CompilerInformation.h"

SMTK_THIRDPARTY_PRE_INCLUDE
#include <boost/uuid/uuid_generators.hpp>
SMTK_THIRDPARTY_POST_INCLUDE

#include <cstdlib> // for getenv()/_dupenv_s()
#include <ctime>   // for time()

namespace
{
// Return true when \a vname exists in the environment (empty or not).
bool checkenv(const char* vname)
{
#if !defined(_WIN32) || defined(__CYGWIN__)
  return getenv(vname) != nullptr;
#else
  char* buf; //allocated or assigned by _dupenv_s
  const bool valid = (_dupenv_s(&buf, nullptr, vname) == 0) && (buf != nullptr);
  free(buf); //perfectly valid to free a nullptr pointer
  return valid;
#endif
}
} // namespace

namespace smtk
{
namespace common
{

class UUIDGenerator::Internal
{
public:
  Internal()
  {
    if (checkenv("SMTK_IN_VALGRIND"))
    {
      // This is a poor technique for seeding or
      // we would initialize this way all the time.
      m_mtseed.seed(static_cast<boost::mt19937::result_type>(time(nullptr)));
      m_randomGenerator = new boost::uuids::basic_random_generator<boost::mt19937>(&m_mtseed);
    }
    else
    {
      m_randomGenerator = new boost::uuids::basic_random_generator<boost::mt19937>;
    }
  }
  ~Internal() { delete m_randomGenerator; }

  boost::mt19937 m_mtseed;
  boost::uuids::basic_random_generator<boost::mt19937>* m_randomGenerator;
  boost::uuids::nil_generator m_nullGenerator;
};

UUIDGenerator::UUIDGenerator()
{
  this->P = new Internal;
}

UUIDGenerator::~UUIDGenerator()
{
  delete this->P;
}

UUID UUIDGenerator::random()
{
  return UUID((*this->P->m_randomGenerator)());
}

/// Generate a nil UUID.
UUID UUIDGenerator::null()
{
  return UUID(this->P->m_nullGenerator());
}

static thread_local UUIDGenerator s_generator;

UUIDGenerator& UUIDGenerator::instance()
{
  return s_generator;
}

} // namespace common
} // namespace smtk
