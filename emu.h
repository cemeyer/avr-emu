#ifndef __EMU_H__
#define __EMU_H__

#include <sys/cdefs.h>

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#define	likely(cond)	__builtin_expect ((cond), 1)
#define	unlikely(cond)	__builtin_expect ((cond), 0)
#ifndef	__unused
#define	__unused	__attribute__((unused))
#endif
#ifndef	__dead2
#define	__dead2		__attribute__((__noreturn__))
#endif

#define	ARRAYLEN(arr)	((sizeof(arr)) / sizeof((arr)[0]))

#define	min(x, y)	({						\
	typeof(x) _min1 = (x);						\
	typeof(y) _min2 = (y);						\
	(void) (&_min1 == &_min2);					\
	_min1 < _min2 ? _min1 : _min2; })

typedef unsigned int uns;

#define	sec		1000000ULL
#define	ptr(X)		((void*)((uintptr_t)X))

#define ASSERT(cond, args...) do {					\
	if (likely(!!(cond)))						\
		break;							\
	printf("%s:%u: ASSERT %s failed: ", __FILE__, __LINE__, #cond);	\
	printf(args);							\
	printf("\n");							\
	abort_nodump();							\
} while (0)

/* Register pairs */
#define	REGP_Z		30
#define	REGP_Y		28
#define	REGP_X		26

/* SREG is accessible in data memory as an IO port */
#define	SREG		0x5F
#define	SP_HI		0x5E
#define	SP_LO		0x5D
#define	EIND		0x5C
#define	RAMPZ		0x5B
#define	RAMPY		0x5A
#define	RAMPX		0x59
#define	RAMPD		0x58
#define	CCP		0x54

#define	IO_BASE		0x20
#define	SREG_IO		(SREG - IO_BASE)

#define	EIO_BASE	0x60

/* Status flags */
#define	SREG_C		0x01u
#define	SREG_Z		0x02u
#define	SREG_N		0x04u
#define	SREG_V		0x08u
#define	SREG_S		0x10u
#define	SREG_H		0x20u
#define	SREG_T		0x40u
#define	SREG_I		0x80u

extern uint32_t		 pc;
extern uint32_t		 pc_start;
extern uint32_t		 instr_size;
extern bool		 skip_next_instruction;
extern uint8_t		 memory[0x1000000];
extern uint16_t		 flash[ 0x1000000 / sizeof(uint16_t)];
extern bool		 pc22;
extern bool		 pc_mem_max_64k;
extern bool		 pc_mem_max_256b;
extern bool		 off;
extern bool		 replay_mode;
extern bool		 stepone;
extern uint64_t		 insns;
extern uint64_t		 insnreplaylim;
extern uint64_t		 insnlimit;

void		 abort_nodump(void) __dead2;
void		 init(void);
void		 destroy(void);
void		 emulate(void);
void		 emulate1(void);
#define	unhandled(instr)	_unhandled(__FILE__, __LINE__, instr)
void		 _unhandled(const char *f, unsigned l, uint16_t instr) __dead2;
#define	illins(instr)		_illins(__FILE__, __LINE__, instr)
void		 _illins(const char *f, unsigned l, uint16_t instr) __dead2;
void		 print_regs(void);
/* Microseconds: */
uint64_t	 now(void);
#ifndef	EMU_CHECK
void		 getsn(uint16_t addr, uint16_t len);
#endif

void		 print_ips(void);

// GDB stuff
void		 gdbstub_init(void);
void		 gdbstub_intr(void);
void		 gdbstub_stopped(void);
void		 gdbstub_interactive(void);
void		 gdbstub_breakpoint(void);

/* Alternative bits() implementation, if the compiler isn't smart enough. */
#if 0
#define	_MASK(N)	((1U << (N)) - 1)
#define	m_bits(N, H, L)	((N) & _MASK(H+1) & ~_MASK(L))
#endif

static inline uint16_t
bits(uint16_t v, unsigned max, unsigned min)
{
	uint16_t mask;

#ifndef	REALLYFAST
	ASSERT(max < 16 && max >= min, "bit-select");
	ASSERT(min < 16, "bit-select");
#endif

	mask = ((unsigned)1 << (max+1)) - 1;
	if (min > 0)
		mask &= ~( (1<<min) - 1 );

	return v & mask;
}

/* The only big-endian register pair on this damn machine */
static inline uint16_t
getsp(void)
{

	return (((uint16_t)memory[SP_HI] << 8) | memory[SP_LO]);
}

static inline void
setsp(uint16_t sp)
{

	memory[SP_LO] = (sp & 0xff);
	memory[SP_HI] = (sp >> 8);
}

static inline uint16_t
membyte(uint32_t addr)
{

#ifndef	REALLYFAST
	ASSERT((addr & ~0xffffff) == 0,
	    "load outside 24-bit addressable data memory: %#x",
	    (unsigned)addr);
#endif
	return (memory[addr]);
}

static inline uint16_t
memword(uint32_t addr)
{

#ifndef	REALLYFAST
	ASSERT((addr & 0x1) == 0, "word load unaligned: %#04x",
	    (unsigned)addr);
	ASSERT((addr & ~0xffffff) == 0,
	    "load outside 24-bit addressable data memory: %#x",
	    (unsigned)addr);
#endif
	return (memory[addr] | ((uint16_t)memory[addr + 1] << 8));
}

static inline void
memwriteword(uint32_t addr, uint16_t word)
{

#ifndef	REALLYFAST
	ASSERT((addr & 0x1) == 0, "word store unaligned: %#04x",
	    (uns)addr);
#endif
	memory[addr] = word & 0xff;
	memory[addr + 1] = (word >> 8) & 0xff;
}

/*
 * Flash is word-addressed (even though some operations select a single byte
 * from that word).  In particular PC addressed a word only.
 */
static inline uint16_t
romword(uint32_t addr_word)
{

#ifndef	REALLYFAST
	ASSERT((addr_word & ~0x7fffff) == 0,
	    "load outside 24-bit addressable program memory: %#x",
	    (unsigned)addr_word);
#endif
	return (flash[addr_word]);
}

#endif
