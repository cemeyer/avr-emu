#include "emu.h"
#include "instr.h"

void
instr_nop(const struct instr_decode_common *idc __unused)
{
}

void
instr_mov(const struct instr_decode_common *idc)
{

	memory[idc->ddddd] = memory[idc->rrrrr];
}
