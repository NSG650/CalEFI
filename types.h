#pragma once

#define EXTERN extern
#define STATIC static
#define NORETURN __declspec(noreturn)
#define BPAPI __stdcall
#define FIELD_OFFSET(x, y)      ( ( ULONG32 )&( ( ( x* )0 )->y ) )

#define C_ASSERT( x ) static_assert( x, #x )

#define VOID void
#define NULL 0

#define LOADER_BASIC_TYPES_DEFINED 

typedef unsigned long long ULONG64, * PULONG64;
typedef unsigned long      ULONG32, * PULONG32;
typedef unsigned short     USHORT, * PUSHORT;
typedef unsigned char      UCHAR, * PUCHAR;

typedef long long          LONG64, * PLONG64;
typedef long               LONG32, * PLONG32;
typedef short              SHORT, * PSHORT;
typedef char               CHAR, * PCHAR, * PSTR;

typedef VOID* PVOID;

typedef short              WCHAR, * PWCHAR;

typedef char* va_list;

typedef long               NTSTATUS;
