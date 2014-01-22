#ifndef EMU_H
#define EMU_H

#include <sys/cdefs.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#define likely(cond) __builtin_expect ((cond), 1)
#define unlikely(cond) __builtin_expect ((cond), 0)

struct taint {
	unsigned	ntaints;
	uint16_t	addrs[0];
};

extern uint16_t		 pc_start;
extern uint16_t		 registers[16];
extern uint8_t		 memory[0x10000];
extern struct taint	*register_taint[16];
extern GHashTable	*memory_taint;		// addr -> struct taint

#define PC 0
#define SP 1
#define SR 2
#define CG 3

#define AS_REG    0x00
#define AS_IDX    0x10
#define AS_REGIND 0x20
#define AS_INDINC 0x30

#define AS_R2_ABS 0x10
#define AS_R2_4   0x20
#define AS_R2_8   0x30

#define AS_R3_0   0x00
#define AS_R3_1   0x10
#define AS_R3_2   0x20
#define AS_R3_NEG 0x30

#define AD_REG    0x00
#define AD_IDX    0x80

#define AD_R2_ABS 0x80

#define SR_CPUOFF 0x0010

enum operand_kind {
	OP_REG,
	OP_MEM,
	OP_CONST,
};

typedef unsigned int uns;

#define ASSERT(cond, args...) do { \
	if (likely(!!(cond))) \
		break; \
	printf("%s:%u: ASSERT %s failed: ", __FILE__, __LINE__, #cond); \
	printf(args); \
	printf("\n"); \
	abort_nodump(); \
} while (false)

void		 abort_nodump(void);
void		 init(void);
void		 destroy(void);
void		 emulate(void);
void		 emulate1(void);
uint16_t	 memword(uint16_t addr);
void		 mem2reg(uint16_t addr, unsigned reg);
void		 reg2mem(unsigned reg, uint16_t addr);
uint16_t	 bits(uint16_t v, unsigned max, unsigned min);
void		 copytaint(struct taint **dest, const struct taint *src);
void		 unhandled(uint16_t instr);
void		 illins(uint16_t instr);
struct taint	*newtaint(void);
void		 inc_reg(uint16_t reg, uint16_t bw);
void		 print_regs(void);
void		 taint_mem(uint16_t addr);
void		 addtaint(struct taint **dst, struct taint *src);
bool		 regtainted(uint16_t reg, uint16_t addr);
bool		 regtaintedexcl(uint16_t reg, uint16_t addr);

void	handle_jump(uint16_t instr);
void	handle_single(uint16_t instr);
void	handle_double(uint16_t instr);

void	load_src(uint16_t instr, uint16_t instr_decode_src,
		 uint16_t As, uint16_t bw, uint16_t *srcval,
		 enum operand_kind *srckind);
void	load_dst(uint16_t instr, uint16_t instr_decode_dst,
		 uint16_t Ad, uint16_t *dstval,
		 enum operand_kind *dstkind);

#endif