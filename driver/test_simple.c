/*
 * test_simple - exercises simple.device Open/BeginIO/Close
 *
 * Verifies that the device:
 *  - is loadable via Exec's romtag scanner (OpenDevice path)
 *  - accepts OpenDevice("simple.device", 0, ...) successfully
 *  - calls BeginIO on a sent IORequest and replies it back
 *  - returns IOERR_NOCMD (-3) for unknown commands
 *  - cleans up after CloseDevice
 *
 * No C runtime, no amiga.lib. Only exec.library and dos.library functions.
 * The MsgPort and IORequest are stack-allocated and initialized by hand
 * (equivalents of CreatePort and CreateExtIO from amiga.lib).
 *
 * IMPORTANT: must be built as separate compile + link steps. A single-step
 * gcc invocation produces a binary where _start does not end up at offset 0
 * in the loaded hunk, and the program silently does nothing.
 *
 * Build:
 *   m68k-amiga-elf-gcc -m68000 -Os -Wall -fomit-frame-pointer \
 *       -Wno-array-bounds -Wno-volatile-register-var \
 *       -I$HOME/AMIGA_GCC/NDK_3.2_elf/sys-include \
 *       -c test_simple.c -o test_simple.o
 *
 *   m68k-amiga-elf-gcc -m68000 -Os \
 *       -o test_simple.elf test_simple.o \
 *       -nostdlib -nostartfiles \
 *       -Wl,--emit-relocs,-Ttext=0
 *
 *   elf2hunk test_simple.elf test_simple
 *
 * Run from AmigaShell after copying simple.device into DEVS:
 * and test_simple into any directory:
 *   1> test_simple
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <exec/errors.h>

struct ExecBase   *SysBase;
struct DosLibrary *DOSBase;  /* storage for the extern declared in <proto/dos.h> */

/* Write a string literal. sizeof(s) - 1 strips the trailing NUL. */
#define PUTS(s) Write(out, (CONST_STRPTR)(s), sizeof(s) - 1)

LONG __attribute__((no_reorder)) _start(void)
{
    SysBase = *(struct ExecBase **)4UL;

    DOSBase = (struct DosLibrary *)
        OpenLibrary((CONST_STRPTR)"dos.library", 0);
    if (!DOSBase)
        return 20;

    BPTR out = Output();
    LONG rc  = 20;

    BYTE sigbit = AllocSignal(-1);
    if (sigbit < 0) {
        PUTS("AllocSignal failed\n");
        goto cleanup_dos;
    }

    /* Reply port -- amiga.lib's CreatePort without the alloc. */
    struct MsgPort mp;
    mp.mp_Node.ln_Type = NT_MSGPORT;
    mp.mp_Node.ln_Pri  = 0;
    mp.mp_Node.ln_Name = NULL;
    mp.mp_Flags        = PA_SIGNAL;
    mp.mp_SigBit       = sigbit;
    mp.mp_SigTask      = FindTask(NULL);
    /* NewList equivalent */
    mp.mp_MsgList.lh_Head     = (struct Node *)&mp.mp_MsgList.lh_Tail;
    mp.mp_MsgList.lh_Tail     = NULL;
    mp.mp_MsgList.lh_TailPred = (struct Node *)&mp.mp_MsgList.lh_Head;
    mp.mp_MsgList.lh_Type     = 0;

    /* IORequest -- amiga.lib's CreateExtIO without the alloc. */
    struct IOStdReq ior;
    ior.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ior.io_Message.mn_Node.ln_Pri  = 0;
    ior.io_Message.mn_Node.ln_Name = NULL;
    ior.io_Message.mn_ReplyPort    = &mp;
    ior.io_Message.mn_Length       = sizeof(ior);

    PUTS("OpenDevice simple.device unit 0... ");
    if (OpenDevice((CONST_STRPTR)"simple.device", 0,
                   (struct IORequest *)&ior, 0) != 0)
    {
        PUTS("FAILED\n");
        goto cleanup_signal;
    }
    PUTS("OK\n");

    /* Exercise BeginIO. The template implements no commands, so any
       command should come back with IOERR_NOCMD. */
    ior.io_Command = CMD_RESET;
    ior.io_Flags   = 0;
    DoIO((struct IORequest *)&ior);

    if (ior.io_Error == IOERR_NOCMD)
        PUTS("CMD_RESET -> IOERR_NOCMD (expected)\n");
    else
        PUTS("CMD_RESET -> unexpected result\n");

    CloseDevice((struct IORequest *)&ior);
    PUTS("CloseDevice done\n");
    rc = 0;

cleanup_signal:
    FreeSignal(sigbit);
cleanup_dos:
    CloseLibrary((struct Library *)DOSBase);
    return rc;
}
