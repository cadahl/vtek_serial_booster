/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef VSERR_H
#define VSERR_H

#include "exec/errors.h"
#include "devices/serial.h"

/*
   Combined error type.
*/
enum VSErr {
    VSErr_OpenFail          = IOERR_OPENFAIL,
    VSErr_Aborted           = IOERR_ABORTED,
    VSErr_NoCMD             = IOERR_NOCMD,
    VSErr_BadLength         = IOERR_BADLENGTH,
    VSErr_BadAddress        = IOERR_BADADDRESS,
    VSErr_UnitBusy          = IOERR_UNITBUSY,
    VSErr_SelfTest          = IOERR_SELFTEST,

    VSErr_Success           = 0,

   // Error codes from devices/serial.h
   VSErr_DevBusy	        = SerErr_DevBusy,
   VSErr_BaudMismatch       = SerErr_BaudMismatch,
   VSErr_BufErr	            = SerErr_BufErr,
   VSErr_InvParam           = SerErr_InvParam,
   VSErr_LineErr	        = SerErr_LineErr,
   VSErr_ParityErr          = SerErr_ParityErr,
   VSErr_TimerErr           = SerErr_TimerErr,
   VSErr_BufOverflow        = SerErr_BufOverflow,
   VSErr_NoDSR	            = SerErr_NoDSR,
   VSErr_DetectedBreak      = SerErr_DetectedBreak,

   // Error codes specific to this driver.
   VSErr_HardwareNotPresent = 64,
   VSErr_TooManyRequests    = 65,

};

typedef enum VSErr vserr_t;

#endif
