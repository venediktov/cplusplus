/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#ifndef DLLEXP
#define DLLEXP __declspec( dllexport )
#endif

#ifdef _MSC_VER

#define assert ASSERT
#if __cplusplus < 199711L
#define snprintf _snprintf
#endif
#define _AFXDLL
#include <afxwin.h>

#endif

