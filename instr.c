#include "emu.h"
#include "instr.h"

static inline void
logic_flags8(uint8_t result, uint8_t *setflags, uint8_t *clrflags)
{

	*clrflags = SREG_V;

	if (result & 0x80)
		*setflags |= (SREG_N | SREG_S);
	else
		*clrflags |= (SREG_N | SREG_S);

	if (result == 0)
		*setflags |= SREG_Z;
	else
		*clrflags |= SREG_Z;
}

void
instr_in(struct instr_decode_common *idc)
{
	uint8_t io_port;

	io_port = (bits(idc->instr, 10, 9) >> 5) | bits(idc->instr, 3, 0);
	memory[idc->ddddd] = memory[IO_BASE + io_port];
}

void
instr_mov(struct instr_decode_common *idc)
{

	memory[idc->ddddd] = memory[idc->rrrrr];
}

void
instr_movw(struct instr_decode_common *idc)
{
	uint8_t RRRR, DDDD;

	DDDD = (idc->ddddd - 16) << 1;
	RRRR = (idc->rrrrr - 16) << 1;
	memwriteword(DDDD, memword(RRRR));
}

void
instr_nop(struct instr_decode_common *idc __unused)
{
}

void
instr_or(struct instr_decode_common *idc)
{

	memory[idc->ddddd] |= memory[idc->rrrrr];
	logic_flags8(memory[idc->ddddd], &idc->setflags, &idc->clrflags);
}

void
instr_ori(struct instr_decode_common *idc)
{

	memory[idc->ddddd] |= idc->imm_u8;
	logic_flags8(memory[idc->ddddd], &idc->setflags, &idc->clrflags);
}

void
instr_out(struct instr_decode_common *idc)
{
	uint8_t io_port;

	io_port = (bits(idc->instr, 10, 9) >> 5) | bits(idc->instr, 3, 0);
	memory[IO_BASE + io_port] = memory[idc->ddddd];
}

void
instr_push(struct instr_decode_common *idc)
{
	uint16_t sp;

	sp = getsp();
	memory[sp] = memory[idc->ddddd];
	setsp(sp - 1);
}

void
instr_unimp(struct instr_decode_common *idc __unused)
{

	unhandled(idc->instr);
}