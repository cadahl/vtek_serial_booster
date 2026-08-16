/*
 * debug.c -- KPrintF for the elf-toolchain
 *
 * Bebbo's amiga-gcc ships clib/debug_protos.h + libdebug.a, which provide
 * KPrintF directly via -ldebug -mcrt=clib2. The Makefile takes that path
 * when TARGET=hunk-toolchain.
 *
 * Bartman's m68k-amiga-elf toolchain ships neither. We synthesize KPrintF
 * here using exec.library's RawDoFmt (printf-style formatting) plus
 * RawPutChar (single-byte serial debug output, LVO -516).
 *
 * Only compiled into DEBUG builds (see Makefile).
 */

#if DEBUG

#include <proto/exec.h>
#include <stdarg.h>

extern struct ExecBase *SysBase;

/*
 * Inline stub for RawPutChar (exec.library LVO -516). Not declared in
 * NDK 3.2 headers (private debug entry point), so we emit the JSR
 * ourselves.
 *
 * Note on percent-escaping: this is gcc extended asm (with operand list),
 * so literal % in instructions must be doubled (%%a6). GNU as for m68k
 * requires the % prefix on register names.
 */
static inline void raw_put_char(UBYTE c)
{
    register UBYTE c_in asm("d0") = c;
    register struct ExecBase *sysbase_in asm("a6") = SysBase;
    asm volatile (
        "jsr %%a6@(-516)"
        :
        : "d" (c_in), "a" (sysbase_in)
        : "d1", "cc", "memory"
    );
}

/*
 * RawDoFmt's per-character callback. Exec calls this with the byte in d0
 * and the user-data pointer in a3.
 */
static void putch(UBYTE c    asm("d0"),
                  APTR  data asm("a3"))
{
    (void)data;
    raw_put_char(c);
}

void KPrintF(CONST_STRPTR fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    RawDoFmt((STRPTR)fmt, ap, (void (*)())putch, NULL);
    va_end(ap);
}

#endif /* DEBUG */
