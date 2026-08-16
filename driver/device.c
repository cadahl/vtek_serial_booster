#include <proto/misc.h>
#include <proto/exec.h>
#include <clib/misc_protos.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/intbits.h>
#include <exec/types.h>
#include <resources/misc.h>
#include <resources/cia.h>
#include <libraries/dos.h>
#include <devices/serial.h>
#include <stdbool.h>
#include <string.h>
#include "buffy.h"
#include "util.h"

extern struct Custom custom;
extern struct CIA ciab;

#define RX_FIFO_LEN 8192
#define TX_FIFO_LEN 8192

#define RRQ_LEN 8

struct VTekSerialDevice {
    struct Library library;

    uint16_t serial_status;

    uint8_t rx_fifo[RX_FIFO_LEN];
    uint8_t rx_fifo_h[RX_FIFO_LEN / 8];

    uint8_t tx_fifo[TX_FIFO_LEN];
    uint8_t tx_fifo_h[TX_FIFO_LEN / 8];

    uint16_t rx_fifo_read_index;
    uint16_t rx_fifo_write_index;

    uint16_t tx_fifo_read_index;
    uint16_t tx_fifo_write_index;

    uint8_t host_state;
    struct Interrupt vbl_interrupt;
    bool is_nine_bits;
    bool is_serialport_owned;
    bool is_serialbits_owned;
    bool is_interrupt_server_installed;

    struct IOExtSer *rrq[RRQ_LEN];
    uint16_t rrq_read_index;
    uint16_t rrq_write_index;
};

static void reset_paula(void);
static void update_serial_status_from_cia(struct VTekSerialDevice *vsdev);
static uint16_t get_tx_fifo_len(struct VTekSerialDevice *vsdev);
static void vbl_handler(struct VTekSerialDevice *vsdev asm("a1"));

static void rx_fifo_reset(struct VTekSerialDevice *vsdev);
static void rx_fifo_dequeue_n(struct VTekSerialDevice *vsdev, uint8_t *buffer, uint16_t len);
static uint16_t rx_fifo_get_length(struct VTekSerialDevice *vsdev);

static void tx_fifo_reset(struct VTekSerialDevice *vsdev);

static bool rrq_enqueue(struct VTekSerialDevice *vsdev, struct IOExtSer *ioextser);
static void rrq_try_dequeue_one(struct VTekSerialDevice *vsdev);

#if DEBUG
/* KPrintF is provided either by libdebug.a (Bebbo's toolchain, linked via
   -ldebug -mcrt=clib2) or by debug.c in this repo (Bartman's elf-toolchain,
   which has no libdebug). Either way we just declare the prototype here. */
extern void KPrintF(CONST_STRPTR fmt, ...);
#endif

#define STR(s) #s      /* Turn s into a string literal without expanding macro definitions (however, \
                          if invoked from a macro, macro arguments are expanded). */
#define XSTR(s) STR(s) /* Turn s into a string literal after macro-expanding it. */

#define DEVICE_NAME "vtekser.device"
#define DEVICE_DATE "(16 Aug 2026)"
#define DEVICE_ID_STRING "VTek Serial " XSTR(DEVICE_VERSION) "." XSTR(DEVICE_REVISION) " " DEVICE_DATE /* format is: 'name version.revision (d.m.yy)' */
#define DEVICE_VERSION 1
#define DEVICE_REVISION 0
#define DEVICE_PRIORITY 0 /* Most people will not need a priority and should leave it at zero. */

struct ExecBase *SysBase;
BPTR saved_seg_list;
static bool is_open = false;

/*-----------------------------------------------------------
A library or device with a romtag should start with moveq #-1,d0 (to
safely return an error if a user tries to execute the file), followed by a
Resident structure.
------------------------------------------------------------*/
int __attribute__((no_reorder)) _start()
{
    return -1;
}

/*----------------------------------------------------------- 
A romtag structure.  After your driver is brought in from disk, the
disk image will be scanned for this structure to discover magic constants
about you (such as where to start running you from...).

endcode is a marker that shows the end of your code. Make sure it does not
span hunks, and is not before the rom tag! It is ok to put it right after
the rom tag -- that way you are always safe.
Make sure your program has only a single code hunk if you put it at the 
end of your code.
------------------------------------------------------------*/
asm("romtag:                                \n"
    "       dc.w    "XSTR(RTC_MATCHWORD)"   \n"
    "       dc.l    romtag                  \n"
    "       dc.l    endcode                 \n"
    "       dc.b    "XSTR(RTF_AUTOINIT)"    \n"
    "       dc.b    "XSTR(DEVICE_VERSION)"  \n"
    "       dc.b    "XSTR(NT_DEVICE)"       \n"
    "       dc.b    "XSTR(DEVICE_PRIORITY)" \n"
    "       dc.l    _device_name            \n"
    "       dc.l    _device_id_string       \n"
    "       dc.l    _auto_init_tables       \n"
    "endcode:                               \n");

char device_name[] = DEVICE_NAME;
char device_id_string[] = DEVICE_ID_STRING;

extern struct Library *MiscBase;

static BPTR do_expunge(struct Library *dev) {
#if DEBUG
    KPrintF((CONST_STRPTR) "running do_expunge()\n");
#endif
    struct VTekSerialDevice *vsdev = (struct VTekSerialDevice *)dev;

    if (dev->lib_OpenCnt != 0) {
        dev->lib_Flags |= LIBF_DELEXP;
        return 0;
    }

    if (vsdev->is_serialport_owned) {
        FreeMiscResource(MR_SERIALPORT);
        vsdev->is_serialport_owned = false;
    }

    if (vsdev->is_serialbits_owned) {
        FreeMiscResource(MR_SERIALBITS);
        vsdev->is_serialbits_owned = false;
    }

    if (vsdev->is_interrupt_server_installed) {
        RemIntServer(INTB_VERTB, &vsdev->vbl_interrupt);
        vsdev->is_interrupt_server_installed = false;
    }

    BPTR seg_list = saved_seg_list;
    Remove(&dev->lib_Node);
    FreeMem((char *)dev - dev->lib_NegSize, dev->lib_NegSize + dev->lib_PosSize);
    return seg_list;
}

static void do_open(struct Library *dev, struct IORequest *ioreq, ULONG unitnum, ULONG flags)
{
#if DEBUG
    KPrintF((CONST_STRPTR) "running do_open()\n");
#endif

    struct VTekSerialDevice *vsdev = (struct VTekSerialDevice *)dev;

    ioreq->io_Error = IOERR_OPENFAIL;
    ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

    if (unitnum != 0) {
        return;
    }

    if (is_open) {
        ioreq->io_Error = SerErr_DevBusy;
        return;
    }

    if (!is_open) {
        if (!buffy_open()) {
            // No Buffy chip or something else went wrong.
            return;
        }

        if (!MiscBase) {
            MiscBase = OpenResource((CONST_STRPTR)"misc.resource");
            if (!MiscBase) {
                return;
            }
        }

        if (!vsdev->is_serialport_owned) {
            UBYTE *serialport_owner = AllocMiscResource(MR_SERIALPORT, (CONST_STRPTR)DEVICE_NAME);
            if (serialport_owner != NULL) {
                ioreq->io_Error = SerErr_DevBusy;
                return;
            }
            vsdev->is_serialport_owned = true;
        }

        if (!vsdev->is_serialbits_owned) {
            UBYTE *serialbits_owner = AllocMiscResource(MR_SERIALBITS, (CONST_STRPTR)DEVICE_NAME);
            if (serialbits_owner != NULL) {
                ioreq->io_Error = SerErr_DevBusy;
                return;
            }
            vsdev->is_serialbits_owned = true;
        }

        reset_paula();
        rx_fifo_reset(vsdev);
        tx_fifo_reset(vsdev);
        update_serial_status_from_cia(vsdev);

        if (!vsdev->is_interrupt_server_installed) {
            vsdev->vbl_interrupt.is_Node.ln_Type = NT_INTERRUPT;
            vsdev->vbl_interrupt.is_Node.ln_Pri  = 0;
            vsdev->vbl_interrupt.is_Node.ln_Name = DEVICE_NAME;
            vsdev->vbl_interrupt.is_Data = vsdev;
            vsdev->vbl_interrupt.is_Code = (APTR)vbl_handler;
            AddIntServer(INTB_VERTB, &vsdev->vbl_interrupt);
            vsdev->is_interrupt_server_installed = true;
        }

        is_open = TRUE;
    }

    dev->lib_OpenCnt++;
    ioreq->io_Error = 0; //Success
}

static BPTR do_close(struct Library *dev, struct IORequest *ioreq)
{
#if DEBUG
    KPrintF((CONST_STRPTR) "running do_close()\n");
#endif

    buffy_close();

    ioreq->io_Device = NULL;
    ioreq->io_Unit = NULL;

    dev->lib_OpenCnt--;

    if (dev->lib_OpenCnt == 0 && (dev->lib_Flags & LIBF_DELEXP)) {
        return do_expunge(dev);
    }

    return 0;
}

static void do_begin_io(struct Library *dev, struct IORequest *ioreq)
{
    struct IOExtSer *ioextser = (struct IOExtSer *)ioreq;
    struct VTekSerialDevice *vsdev = (struct VTekSerialDevice *)dev;

#if DEBUG
    KPrintF((CONST_STRPTR) "running do_begin_io()\n");
#endif

    switch (ioreq->io_Command) {
        case CMD_CLEAR: {
            break;
        }
        case CMD_RESET: {
            RemIntServer(INTB_VERTB, &vsdev->vbl_interrupt);

            buffy_reset();
            reset_paula();
            rx_fifo_reset(vsdev);
            tx_fifo_reset(vsdev);

            AddIntServer(INTB_VERTB, &vsdev->vbl_interrupt);
            ioreq->io_Error = 0;
            break;
        }
        case CMD_READ: {
            const ULONG len = ioextser->IOSer.io_Length;
            const uint16_t available_len = rx_fifo_get_length(vsdev);
            if (len > available_len) {
                if (!rrq_enqueue(vsdev, ioextser)) {
                    // No more queue slots!
                    ioreq->io_Error = SerErr_BufErr;
                    return;
                }
            } else {
                rx_fifo_dequeue_n(vsdev, ioextser->IOSer.io_Data, len);
                ioextser->IOSer.io_Actual = len;
                ioreq->io_Error = 0;
            }
            break;
        }
        case CMD_WRITE: {
            LONG len = ioextser->IOSer.io_Length;
            const uint8_t *in_buffer = (uint8_t *)ioextser->IOSer.io_Data;
            if (len < 0) {
                len = strlen((const char *)in_buffer);
            }
            const uint16_t available_len = TX_FIFO_LEN - get_tx_fifo_len(vsdev);
            if (len > available_len) {
                ioreq->io_Error = SerErr_BufOverflow;
                return;
            }
            for (LONG i = 0; i < len; ++i) {
                const uint8_t data = *in_buffer++;
                const uint16_t next_write_index = (vsdev->tx_fifo_write_index + 1) & (TX_FIFO_LEN - 1);
                if (next_write_index == vsdev->tx_fifo_read_index) {
                    // FULL
                } else {
                    vsdev->tx_fifo[vsdev->tx_fifo_write_index] = data;
                    vsdev->tx_fifo_write_index = next_write_index;
                }
            }
            ioextser->IOSer.io_Actual = len;
            ioreq->io_Error = 0;
            break;
        }
        case SDCMD_SETPARAMS: {
            const uint8_t serflags = ioextser->io_SerFlags;

            // We don't do sharing or parity.
            if (serflags & (SERF_SHARED | SERF_PARTY_ON)) {
                ioreq->io_Error = SerErr_InvParam;
                break;
            }

            // XON/XOFF
            if (serflags & SERF_XDISABLED) {
                vsdev->host_state &= ~BUFFY_HOST_STATE_XONXOFF_ENABLED;
            } else {
                vsdev->host_state |= BUFFY_HOST_STATE_XONXOFF_ENABLED;
            }

            // Character length
            const uint16_t read_len = ioextser->io_ReadLen;
            if (read_len < 8 || read_len > 9) {
                ioreq->io_Error = SerErr_InvParam;
                break;
            } 

            uint32_t serper = (uint16_t)((((3546895ULL << 20ULL) / ioextser->io_Baud) >> 20ULL) - 1);

            if (serper > 0x7EFF) {
                serper = 0x7EFF;
            }

            if (ioextser->io_ReadLen == 9) {
                serper |= 0x8000;
                vsdev->is_nine_bits = true;
            } else {
                vsdev->is_nine_bits = false;
            }

            custom.serper = (uint16_t)serper;
            wait_us(64);

            custom.serper = BUFFY_HOST_STATE_PREFIX | vsdev->host_state;

            ioreq->io_Error = 0;
            break;
        }
        case SDCMD_QUERY: {
            ioextser->IOSer.io_Actual = rx_fifo_get_length(vsdev);
            update_serial_status_from_cia(vsdev);
            ioextser->io_Status = vsdev->serial_status;
            ioreq->io_Error = 0;
            break;
        }
    }

    /* If IOF_QUICK is set, the caller wants a synchronous return; leave the
       flag set and skip ReplyMsg. Otherwise ReplyMsg the request back. */

    if (!(ioreq->io_Flags & IOF_QUICK)) {
        ReplyMsg(&ioreq->io_Message);
    }
}

static ULONG do_abort_io(struct Library *dev, struct IORequest *ioreq)
{
#if DEBUG
    KPrintF((CONST_STRPTR) "running do_abort_io()\n");
#endif

    return IOERR_NOCMD;
}

/*------- init_device ---------------------------------------
FOR RTF_AUTOINIT:
  This routine gets called after the device has been allocated.
  Exec passes ExecBase in a6, the AmigaDOS segment list in a0, and
  the device pointer in d0. If it returns the device pointer, then
  the device will be linked into the device list. If it returns NULL,
  then the device will be unloaded.

IMPORTANT:
  If you don't use the "RTF_AUTOINIT" feature, there is an additional
  caveat. If you allocate memory in your Open function, remember that
  allocating memory can cause an Expunge... including an expunge of your
  device. This must not be fatal. The easy solution is don't add your
  device to the list until after it is ready for action.

CAUTION: 
This function runs in a forbidden state !!!                   
This call is single-threaded by Exec
------------------------------------------------------------*/
static struct Library __attribute__((used)) * init_device(struct ExecBase *sys_base asm("a6"), BPTR seg_list asm("a0"), struct Library *dev asm("d0"))
{
    /* !!! required !!! save a pointer to exec FIRST -- before any DBG/KPrintF
       call, since our debug stub uses SysBase to call RawDoFmt/RawPutChar.
       Exec passes ExecBase in a6, so we take it as a parameter rather than
       reading absolute address 4 (avoids gcc 12+'s -Warray-bounds null-deref
       warning). */
    SysBase = sys_base;

#if DEBUG
    KPrintF((CONST_STRPTR) "running init_device()\n");
#endif

    /* save pointer to our loaded code (the SegList) */
    saved_seg_list = seg_list;

    dev->lib_Node.ln_Type = NT_DEVICE;
    dev->lib_Node.ln_Name = device_name;
    dev->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    dev->lib_Version = DEVICE_VERSION;
    dev->lib_Revision = DEVICE_REVISION;
    dev->lib_IdString = (APTR)device_id_string;

    is_open = FALSE;

    return dev;
}

/* device dependent expunge function 
!!! CAUTION: This function runs in a forbidden state !!! 
This call is guaranteed to be single-threaded; only one task 
will execute your Expunge at a time. */
static BPTR __attribute__((used)) expunge(struct Library *dev asm("a6"))
{
    return do_expunge(dev);
}

/* device dependent open function 
!!! CAUTION: This function runs in a forbidden state !!!
This call is guaranteed to be single-threaded; only one task 
will execute your Open at a time. */
static void __attribute__((used)) open(struct Library *dev asm("a6"), struct IORequest *ioreq asm("a1"), ULONG unitnum asm("d0"), ULONG flags asm("d1"))
{
    do_open(dev, ioreq, unitnum, flags);
}

/* device dependent close function 
!!! CAUTION: This function runs in a forbidden state !!!
This call is guaranteed to be single-threaded; only one task 
will execute your Close at a time. */
static BPTR __attribute__((used)) close(struct Library *dev asm("a6"), struct IORequest *ioreq asm("a1"))
{
    return do_close(dev, ioreq);
}

/* device dependent beginio function */
static void __attribute__((used)) begin_io(struct Library *dev asm("a6"), struct IORequest *ioreq asm("a1"))
{
    do_begin_io(dev, ioreq);
}

/* device dependent abortio function */
static ULONG __attribute__((used)) abort_io(struct Library *dev asm("a6"), struct IORequest *ioreq asm("a1"))
{
    return do_abort_io(dev, ioreq);
}

static ULONG device_vectors[] =
    {
        (ULONG)open,
        (ULONG)close,
        (ULONG)expunge,
        0, //extFunc not used here
        (ULONG)begin_io,
        (ULONG)abort_io,
        -1}; //function table end marker

/*-----------------------------------------------------------
The romtag specified that we were "RTF_AUTOINIT".  This means
that the RT_INIT structure member points to one of these
tables below. If the AUTOINIT bit was not set then RT_INIT
would point to a routine to run. 

MyDev_Sizeof    data space size
device_vectors  pointer to function initializers
dataTable       pointer to data initializers
init_device     routine to run
------------------------------------------------------------*/
const ULONG auto_init_tables[4] = {
    sizeof(struct VTekSerialDevice),
    (ULONG)device_vectors,
    0,
    (ULONG)init_device
};




static void reset_paula(void) {
    // Disable RBF and TBE interrupts.
    custom.intena = INTF_RBF | INTF_TBE;

    // Set default baud rate.
    custom.serper = 0x001F;
}

static void rx_fifo_reset(struct VTekSerialDevice *vsdev) {
    vsdev->rx_fifo_read_index = 0;
    vsdev->rx_fifo_write_index = 0;
}

static void tx_fifo_reset(struct VTekSerialDevice *vsdev) {
    vsdev->tx_fifo_read_index = 0;
    vsdev->tx_fifo_write_index = 0;
}

static void update_serial_status_from_cia(struct VTekSerialDevice *vsdev) {
    vsdev->serial_status = (vsdev->serial_status & 0xFF00) | (ciab.ciapra & 0xFC);
}

static uint16_t rx_fifo_get_length(struct VTekSerialDevice *vsdev) {
    const uint32_t ri = vsdev->rx_fifo_read_index;
    const uint32_t wi = vsdev->rx_fifo_write_index < ri ? vsdev->rx_fifo_write_index + RX_FIFO_LEN : vsdev->rx_fifo_write_index;
    return wi - ri;
}

static uint16_t get_tx_fifo_len(struct VTekSerialDevice *vsdev) {
    const uint32_t ri = vsdev->tx_fifo_read_index;
    const uint32_t wi = vsdev->tx_fifo_write_index < ri ? vsdev->tx_fifo_write_index + TX_FIFO_LEN : vsdev->tx_fifo_write_index;
    return wi - ri;
}

static inline void try_rx(struct VTekSerialDevice *vsdev) {
    const uint16_t serdatr = custom.serdatr;
    if ((serdatr & SERDATR_RBF) == 0) {
        return;
    }
    const uint16_t write_index = vsdev->rx_fifo_write_index;
    const uint16_t next_write_index = (write_index + 1) & (RX_FIFO_LEN - 1);
    if (next_write_index == vsdev->rx_fifo_read_index) {
        // FIFO full!
        return;
    }
    vsdev->rx_fifo[write_index] = serdatr;
    if (vsdev->is_nine_bits) {
        const uint8_t mask = 1 << (write_index & 7);
        if (serdatr & 0x100) {
            vsdev->rx_fifo_h[write_index >> 3] |= mask;
        } else {
            vsdev->rx_fifo_h[write_index >> 3] &= ~mask;
        }
    }
    vsdev->rx_fifo_write_index = next_write_index;
}

static inline void try_tx(struct VTekSerialDevice *vsdev) {
    const uint16_t read_index = vsdev->tx_fifo_read_index;
    
    if (read_index == vsdev->tx_fifo_write_index) {
        // Nothing to send.
        return;
    }

    const uint16_t intreqr = custom.intreqr;
    if ((intreqr & INTF_TBE) == 0) {
        // Can't send yet.
        return;
    }

    uint16_t data = vsdev->tx_fifo[read_index];
    vsdev->tx_fifo_read_index = (read_index + 1) & (TX_FIFO_LEN - 1);

    if (vsdev->is_nine_bits) {
        const uint8_t mask = 1 << (read_index & 7);
        const uint8_t data_h = vsdev->tx_fifo_h[read_index >> 3];
        if (data_h & mask) {
            data |= 0x300;
        } else {
            data |= 0x200;
        }
    } else {
        data |= 0x100;
    }

    custom.serdat = data;
}

static void __attribute__((used)) vbl_handler(struct VTekSerialDevice *vsdev asm("a1")) {
    // TODO: set a vblank budget and keep to it. Right now we poll as much as we can.
    // TODO: find out the actual visible area in vpos numbers and compare against these boundaries.

    unsigned short oldSR;
    #define DISABLE_INTERRUPTS()     __asm__ volatile ( \
        "move.w %%sr, %0\n\t" \
        "or.w   #0x0700, %%sr" \
        : "=d" (oldSR) \
        : \
        : "cc" \
    );

    #define RESTORE_INTERRUPTS()     __asm__ volatile ( \
        "move.w %0, %%sr" \
        : \
        : "d" (oldSR) \
        : "cc" \
    );

    DISABLE_INTERRUPTS();

    const volatile uint32_t *vposr32 = (const volatile uint32_t *)0xdff004;
    uint32_t vpos = *vposr32 & 0x1FF00;
    while (vpos < 0x2C00) {
        try_rx(vsdev);
        try_tx(vsdev);
        RESTORE_INTERRUPTS();
        DISABLE_INTERRUPTS();
        vpos = *vposr32 & 0x1FF00;
    }

    RESTORE_INTERRUPTS();

    // Now we're outside the VBLANK region, so display DMA is going again.
    // We use this moment to try to complete one queued up CMD_READ request.
    rrq_try_dequeue_one(vsdev);
}

static void rx_fifo_dequeue_n(struct VTekSerialDevice *vsdev, uint8_t *buffer, uint16_t len) {
    for (ULONG i = 0; i < len; ++i) {
        const uint8_t data = vsdev->rx_fifo[vsdev->rx_fifo_read_index];
        vsdev->rx_fifo_read_index = (vsdev->rx_fifo_read_index + 1) & (RX_FIFO_LEN - 1);
        *buffer++ = data;
    }
}

static bool rrq_enqueue(struct VTekSerialDevice *vsdev, struct IOExtSer *ioextser) {
    uint16_t next_write_index = (vsdev->rrq_write_index + 1) & (RRQ_LEN - 1);
    if (next_write_index == vsdev->rrq_read_index) {
        // Queue is full!
        return false;
    } else {
        vsdev->rrq[vsdev->rrq_write_index] = ioextser;
    }
    return true;
}

static void rrq_try_dequeue_one(struct VTekSerialDevice *vsdev) {
    if (vsdev->rrq_read_index == vsdev->rrq_write_index) {
        // Queue is empty.
        return;
    }
    struct IOExtSer *ioextser = vsdev->rrq[vsdev->rrq_read_index];
    ULONG requested_len = ioextser->IOSer.io_Length;

    if (rx_fifo_get_length(vsdev) < requested_len) {
        // Not enough bytes in RX FIFO.
        return;
    }

    rx_fifo_dequeue_n(vsdev, ioextser->IOSer.io_Data, requested_len);
    ioextser->IOSer.io_Actual = requested_len;

    vsdev->rrq_read_index = (vsdev->rrq_read_index + 1) & (RRQ_LEN - 1);
}
