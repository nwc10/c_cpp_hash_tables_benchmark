namespace cyrus_classic_C {
#include "cyrus_api.h"
#include "hashu64.h"
}

template< typename > struct cyrus_classic
{
  static constexpr const char *label = "cyrus_classic";
  static constexpr const char *color = "rgb( 81, 169, 240 )";
  static constexpr bool tombstone_like_mechanism = false;
};

#include "cyrus_specialisation.h"

#ifdef UINT64_UINT64_MURMUR_ENABLED

CYRUS_SPECIALIZATION( uint64_uint64_murmur, hashu64, cyrus_classic_C )

#endif

#ifdef UINT64_STRUCT448_MURMUR_ENABLED

#error Does not exist

#endif

#ifdef CSTRING_UINT64_FNV1A_ENABLED

CYRUS_SPECIALIZATION( cstring_uint64_fnv1a, hash, cyrus_classic_C )

#endif
