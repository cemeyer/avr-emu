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
