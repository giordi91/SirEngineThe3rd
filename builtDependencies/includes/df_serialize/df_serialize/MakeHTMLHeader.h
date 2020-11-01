// This creates a function "bool WriteHTML(const char* fileName)" which will write the details
// of schemas to an html file.

#include "_common.h"
#include <stdio.h>

inline bool WriteHTML(const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "w+b");
    if (!file)
        return false;

    TSTRING enums, structs, variants;

// Enums

#define ENUM_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION) enums += \
    "<b>" #_NAME " : " _DESCRIPTION "</b><br/><br/>\n" \
    "<table>\n" \
    "<tr><th colspan=2>" #_NAMESPACE "::" #_NAME "</th></tr>\n" \
    ;

#define ENUM_ITEM(_NAME, _DESCRIPTION) enums += \
    "<tr><td>" #_NAME "</td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define ENUM_END() enums += \
    "</table>\n" \
    "<br/>\n" \
    "\n" \
    ;

// Structs

#define STRUCT_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION) structs += \
    "<b>" #_NAME " : " _DESCRIPTION "</b><br/><br/>\n" \
    "<table>\n" \
    "<tr><th colspan=3>" #_NAMESPACE "::" #_NAME "</th></tr>\n" \
    ;

#define STRUCT_INHERIT_BEGIN(_NAMESPACE, _NAME, _BASE, _DESCRIPTION) structs += \
    "<b>" #_NAME " : " _DESCRIPTION "</b><br/><br/>\n" \
    "<table>\n" \
    "<tr><th colspan=3>" #_NAMESPACE "::" #_NAME " - Inherits from " #_BASE "</th></tr>\n" \
    ;

#define STRUCT_FIELD(_TYPE, _NAME, _DEFAULT, _DESCRIPTION) structs += \
    "<tr><td>" #_TYPE " " #_NAME "</td><td>" #_DEFAULT "</td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define STRUCT_FIELD_NO_SERIALIZE(_TYPE, _NAME, _DEFAULT, _DESCRIPTION) structs += \
    "<tr><td><i>" #_TYPE " " #_NAME "</i></td><td>" #_DEFAULT "</td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define STRUCT_DYNAMIC_ARRAY(_TYPE, _NAME, _DESCRIPTION) structs += \
    "<tr><td>" #_TYPE " " #_NAME "[]</td><td></td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define STRUCT_STATIC_ARRAY(_TYPE, _NAME, _SIZE, _DEFAULT, _DESCRIPTION) structs += \
    "<tr><td>" #_TYPE " " #_NAME "[" #_SIZE "]</td><td>" STRIPCOMMASTRING(_DEFAULT) "</td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define STRUCT_END() structs += \
    "</table>\n" \
    "<br/>\n" \
    "\n" \
    ;

// Variants

#define VARIANT_BEGIN(_NAMESPACE, _NAME, _DESCRIPTION) variants += \
    "<b>" #_NAME " : " _DESCRIPTION "</b><br/><br/>\n" \
    "<table>\n" \
    "<tr><th colspan=3>" #_NAMESPACE "::" #_NAME "</th></tr>\n" \
    ;

#define VARIANT_TYPE(_TYPE, _NAME, _DEFAULT, _DESCRIPTION) variants += \
    "<tr><td>" #_TYPE " " #_NAME "</td><td>" #_DEFAULT "</td><td>" _DESCRIPTION "</td></tr>\n" \
    ;

#define VARIANT_END() variants += \
    "</table>\n" \
    "<br/>\n" \
    "\n" \
    ;
