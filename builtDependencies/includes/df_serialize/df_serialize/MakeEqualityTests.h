// This makes member wise equality testing

#pragma once

#include "_common.h"

// Enums - they already have a == operator built in

#define ENUM_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION)

#define ENUM_ITEM(_NAME, _DESCRIPTION)

#define ENUM_END()

// Structs

#define STRUCT_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION) \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B); \
    bool operator != (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    { \
        return !(A==B); \
    } \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    {

#define STRUCT_INHERIT_BEGIN(_NAMESPACE, _NAME, _BASE, _DESCRIPTION) \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B); \
    bool operator != (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    { \
        return !(A==B); \
    } \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    { \
        if (*(const _BASE*)&A != *(const _BASE*)&B) \
            return false;

#define STRUCT_FIELD(_TYPE, _NAME, _DEFAULT, _DESCRIPTION) \
        if (A._NAME != B._NAME) \
            return false;

// No serialize also means no equality test
#define STRUCT_FIELD_NO_SERIALIZE(_TYPE, _NAME, _DEFAULT, _DESCRIPTION)

#define STRUCT_DYNAMIC_ARRAY(_TYPE, _NAME, _DESCRIPTION) \
        if (TDYNAMICARRAY_SIZE(A._NAME) != TDYNAMICARRAY_SIZE(B._NAME)) \
            return false; \
        for (size_t index = 0; index < TDYNAMICARRAY_SIZE(A._NAME); ++index) \
        { \
            if (A._NAME[index] != B._NAME[index]) \
                return false; \
        }

#define STRUCT_STATIC_ARRAY(_TYPE, _NAME, _SIZE, _DEFAULT, _DESCRIPTION) \
        for (size_t index = 0; index < _SIZE; ++index) \
        { \
            if (A._NAME[index] != B._NAME[index]) \
                return false; \
        }

#define STRUCT_END() \
        return true; \
    }

// Variants

#define VARIANT_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION) \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B); \
    bool operator != (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    { \
        return !(A==B); \
    } \
    bool operator == (const _NAMESPACE::_NAME& A, const _NAMESPACE::_NAME& B) \
    { \
        typedef _NAMESPACE::_NAME ThisType; \
        if (A._index != B._index) \
            return false;

#define VARIANT_TYPE(_TYPE, _NAME, _DEFAULT, _DESCRIPTION) \
        if (A._index == ThisType::c_index_##_NAME) \
            return  A._NAME == B._NAME;

#define VARIANT_END() \
        return true; \
    }