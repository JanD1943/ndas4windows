/*++

Copyright (c) 1989-2002  Microsoft Corporation

Module Name:

    mspyKern.h

Abstract:
    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel.

Environment:

    Kernel mode

--*/
#ifndef __MSPYKERN_H__
#define __MSPYKERN_H__

#include <fltKernel.h>
#if WINVER >= 0x6000
#include <dontuse.h>
#include <suppress.h>
#endif
#include "minispy.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  Memory allocation tag
//

#define SPY_TAG 'ypSM'

//
//  Longhorn define for including transaction support
//

#ifndef NTDDI_VERSION
#define MINISPY_LONGHORN 0
#define MINISPY_NOT_W2K  0
#else
#define MINISPY_LONGHORN (NTDDI_VERSION >= NTDDI_LONGHORN)
#define MINISPY_NOT_W2K  (OSVER(NTDDI_VERSION) > NTDDI_WIN2K)
#endif

//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------

typedef struct _MINISPY_DATA {

    //
    //  The object that identifies this driver.
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  The filter that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER Filter;

    //
    //  Server port: user mode connects to this port
    //

    PFLT_PORT ServerPort;

    //
    //  Client connection port: only one connection is allowed at a time.,
    //

    PFLT_PORT ClientPort;

    //
    //  List of buffers with data to send to user mode.
    //

    KSPIN_LOCK OutputBufferLock;
    LIST_ENTRY OutputBufferList;

    //
    //  Lookaside list used for allocating buffers.
    //

    NPAGED_LOOKASIDE_LIST FreeBufferList;

    //
    //  Variables used to throttle how many records buffer we can use
    //

    LONG MaxRecordsToAllocate;
#ifndef NTDDI_VERSION
    LONG RecordsAllocated;
#else
    __volatile LONG RecordsAllocated;
#endif
    //
    //  static buffer used for sending an "out-of-memory" message
    //  to user mode.
    //

#ifndef NTDDI_VERSION
	ULONG StaticBufferInUse;
#else
    __volatile ULONG StaticBufferInUse;
#endif

    //
    //  We need to make sure this buffer aligns on a PVOID boundary because
    //  minispy casts this buffer to a RECORD_LIST structure.
    //  That can cause alignment faults unless the structure starts on the
    //  proper PVOID boundary
    //

    PVOID OutOfMemoryBuffer[RECORD_SIZE/sizeof( PVOID )];

    //
    //  Variable and lock for maintaining LogRecord sequence numbers.
    //

#ifndef NTDDI_VERSION
	ULONG LogSequenceNumber;
#else
    __volatile ULONG LogSequenceNumber;
#endif

    //
    //  The name query method to use.  By default, it is set to
    //  FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, but it can be overridden
    //  by a setting in the registery.
    //

    ULONG NameQueryMethod;

    //
    //  Global debug flags
    //

    ULONG DebugFlags;

#if MINISPY_LONGHORN

    //
    //  Dynamically imported Filter Mgr APIs
    //

    NTSTATUS
    (*PFltSetTransactionContext)(
        __in PFLT_INSTANCE Instance,
        __in PKTRANSACTION Transaction,
        __in FLT_SET_CONTEXT_OPERATION Operation,
        __in PFLT_CONTEXT NewContext,
        __deref_opt_out PFLT_CONTEXT *OldContext
        );

    NTSTATUS
    (*PFltGetTransactionContext)(
        __in PFLT_INSTANCE Instance,
        __in PKTRANSACTION Transaction,
        __deref_out PFLT_CONTEXT *Context
        );

    NTSTATUS
    (*PFltEnlistInTransaction)(
        __in PFLT_INSTANCE Instance,
        __in PKTRANSACTION Transaction,
        __in PFLT_CONTEXT TransactionContext,
        __in NOTIFICATION_MASK NotificationMask
        );

#endif

} MINISPY_DATA, *PMINISPY_DATA;


//
//  Defines the minispy context structure
//

typedef struct _MINISPY_TRANSACTION_CONTEXT {

    ULONG Count;

}MINISPY_TRANSACTION_CONTEXT, *PMINISPY_TRANSACTION_CONTEXT;


//
//  Minispy's global variables
//

extern MINISPY_DATA MiniSpyData;

#define MINI_DEFAULT_MAX_RECORDS_TO_ALLOCATE     500
#define MAX_RECORDS_TO_ALLOCATE             L"MaxRecords"

#define DEFAULT_NAME_QUERY_METHOD           FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP
#define NAME_QUERY_METHOD                   L"NameQueryMethod"

//
//  DebugFlag values
//

#define SPY_DEBUG_PARSE_NAMES   0x00000001

//---------------------------------------------------------------------------
//  Registration structure
//---------------------------------------------------------------------------

extern const FLT_REGISTRATION FilterRegistration;

//---------------------------------------------------------------------------
//  Function prototypes
//---------------------------------------------------------------------------

FLT_PREOP_CALLBACK_STATUS
SpyPreOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SpyPostOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
SpyKtmNotificationCallback (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PFLT_CONTEXT TransactionContext,
    __in ULONG TransactionNotification
    );

NTSTATUS
SpyFilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
SpyQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
MiniSpyReadDriverParameters (
    __in PUNICODE_STRING RegistryPath
    );


//---------------------------------------------------------------------------
//  Memory allocation routines
//---------------------------------------------------------------------------

PMINI_RECORD_LIST
MiniSpyAllocateBuffer (
    __out PULONG RecordType
    );

VOID
MiniSpyFreeBuffer (
    __in PVOID Buffer
    );

//---------------------------------------------------------------------------
//  Logging routines
//---------------------------------------------------------------------------
PMINI_RECORD_LIST
MiniSpyNewRecord (
    VOID
    );

VOID
MiniSpyFreeRecord (
    __in PMINI_RECORD_LIST Record
    );

VOID
SpySetRecordName (
    __inout PMINI_LOG_RECORD LogRecord,
    __in PUNICODE_STRING Name
    );

VOID
SpyLogPreOperationData (
    __in PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout PMINI_RECORD_LIST RecordList
    );

VOID
SpyLogPostOperationData (
    __in PFLT_CALLBACK_DATA Data,
    __inout PMINI_RECORD_LIST RecordList
    );

VOID
MiniSpyLogTransactionNotify (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout PMINI_RECORD_LIST RecordList,
    __in ULONG TransactionNotification
    );

VOID
MiniSpyLog (
    __in PMINI_RECORD_LIST RecordList
    );

NTSTATUS
MiniSpyGetLog (
    __out_bcount_part(OutputBufferLength,*ReturnOutputBufferLength) PUCHAR OutputBuffer,
    __in ULONG OutputBufferLength,
    __out PULONG ReturnOutputBufferLength
    );

VOID
SpyEmptyOutputBufferList (
    VOID
    );

VOID
SpyDeleteTxfContext (
    __inout PFLT_CONTEXT  Context,
    __in FLT_CONTEXT_TYPE  ContextType
    );

#endif  //__MSPYKERN_H__
