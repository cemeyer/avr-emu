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

#ifdef REALLYFAST

#define	_MASK(N)	((1U << (N)) - 1)
#define	bits(N, H, L)	((N) & _MASK(H+1) & ~_MASK(L))

#define	memwriteword(addr, word) do {					\
	memory[addr] = (word) >> 8;					\
	memory[addr + 1] = (word) & 0xff;				\
} while (0)
#define	memword(addr)	(ntohs(*(uint16_t*)&memory[(addr) & 0xffffff]))
#define	romword(addr)	(flash[((addr) & 0x7fffff)])

#endif

extern uint32_t		 pc_start;
extern uint8_t		 memory[0x1000000];
extern uint16_t		 flash[ 0x1000000 / sizeof(uint16_t)];
extern bool		 off;
extern bool		 replay_mode;
extern bool		 stepone;
extern uint64_t		 insns;
extern uint64_t		 insnreplaylim;
extern uint64_t		 insnlimit;

void		 abort_nodump(void);
void		 init(void);
void		 destroy(void);
void		 emulate(void);
void		 emulate1(void);
#ifndef	REALLYFAST
uint16_t	 membyte(uint32_t addr);
uint16_t	 memword(uint32_t addr);
uint16_t	 romword(uint32_t addr_word);
void		 memwriteword(uint32_t addr, uint16_t word);
uint16_t	 bits(uint16_t v, unsigned max, unsigned min);
#endif
#define	unhandled(instr)	_unhandled(__FILE__, __LINE__, instr)
void		 _unhandled(const char *f, unsigned l, uint16_t instr);
#define	illins(instr)		_illins(__FILE__, __LINE__, instr)
void		 _illins(const char *f, unsigned l, uint16_t instr);
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

#endif
