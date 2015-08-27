#include "emu.h"
#include "instr.h"

static inline void
asr_flags8(uint8_t res, uint8_t rd, uint8_t *set, uint8_t *clr)
{

	if (res & 0x80)
		*set |= SREG_N;
	else
		*clr |= SREG_N;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;

	if (rd & 0x1)
		*set |= SREG_C;
	else
		*clr |= SREG_C;

	if (((*set & SREG_C) && (*clr & SREG_N)) ||
	    ((*clr & SREG_C) && (*set & SREG_N)))
		*set |= SREG_V;
	else
		*clr |= SREG_V;

	if (((*set & SREG_V) && (*clr & SREG_N)) ||
	    ((*clr & SREG_V) && (*set & SREG_N)))
		*set |= SREG_S;
	else
		*clr |= SREG_S;
}

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

static inline void
mul_flags16(uint16_t res, uint8_t *set, uint8_t *clr)
{

	if ((res & 0x8000) != 0)
		*set |= SREG_C;
	else
		*clr |= SREG_C;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;
}

static inline void
adc_flags8(uint8_t res, uint8_t rd, uint8_t rr, uint8_t *set, uint8_t *clr)
{

	if (res & 0x80)
		*set |= SREG_N;
	else
		*clr |= SREG_N;
	if ((0x80 & rd & rr & ~res) ||
	    (0x80 & ~rd & ~rr & res))
		*set |= SREG_V;
	else
		*clr |= SREG_V;
	if (((*set & SREG_N) && (*clr & SREG_V)) ||
	    ((*clr & SREG_N) && (*set & SREG_V)))
		*set |= SREG_S;
	else
		*clr |= SREG_S;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;

	if ((0x80 & rd & rr) ||
	    (0x80 & rr & ~res) ||
	    (0x80 & ~res & rd))
		*set |= SREG_C;
	else
		*clr |= SREG_C;
	if ((0x08 & rd & rr) ||
	    (0x08 & rr & ~res) ||
	    (0x08 & ~res & rd))
		*set |= SREG_H;
	else
		*clr |= SREG_H;
}

static inline void
adiw_flags16(uint16_t res, uint16_t rd, uint8_t *set, uint8_t *clr)
{

	if (res & 0x8000)
		*set |= SREG_N;
	else
		*clr |= SREG_N;

	if (0x8000 & ~rd & res)
		*set |= SREG_V;
	else
		*clr |= SREG_V;

	if (((*set & SREG_N) && (*clr & SREG_V)) ||
	    ((*clr & SREG_N) && (*set & SREG_V)))
		*set |= SREG_S;
	else
		*clr |= SREG_S;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;

	if (0x8000 & ~res & rd)
		*set |= SREG_C;
	else
		*clr |= SREG_C;
}

static inline void
pushbyte(uint8_t b)
{
	uint16_t sp;

	sp = getsp();
	memory[sp] = b;
	setsp(sp - 1);
}

void
instr_adc(struct instr_decode_common *idc)
{
	bool carry_mode;
	uint8_t rr, rd, res;

	carry_mode = bits(idc->instr, 12, 12);

	rr = memory[idc->rrrrr];
	rd = memory[idc->ddddd];

	res = rd + rr;
	if (carry_mode && (memory[SREG] & SREG_C) != 0)
		res += 1;

	memory[idc->ddddd] = res;
	adc_flags8(res, rd, rr, &idc->setflags, &idc->clrflags);
}

void
instr_adiw(struct instr_decode_common *idc)
{
	uint16_t imm, res, rd;
	uint8_t dd;

	dd = 24 + (bits(idc->instr, 5, 4) >> 3);
	imm = (bits(idc->instr, 7, 6) >> 2) | bits(idc->instr, 3, 0);

	rd = memword(dd);
	res = rd + imm;
	memwriteword(dd, res);

	adiw_flags16(res, rd, &idc->setflags, &idc->clrflags);
}

void
instr_and(struct instr_decode_common *idc)
{

	memory[idc->ddddd] &= memory[idc->rrrrr];
	logic_flags8(memory[idc->ddddd], &idc->setflags, &idc->clrflags);
}

void
instr_andi(struct instr_decode_common *idc)
{

	memory[idc->ddddd] &= idc->imm_u8;
	logic_flags8(memory[idc->ddddd], &idc->setflags, &idc->clrflags);
}

void
instr_asr(struct instr_decode_common *idc)
{
	uint8_t rd, res;

	rd = memory[idc->ddddd];
	res = (rd >> 1) | (0x80 & rd);
	memory[idc->ddddd] = res;

	asr_flags8(res, rd, &idc->setflags, &idc->clrflags);
}

void
instr_call(struct instr_decode_common *idc)
{
	uint32_t addr, pc2;

	addr = idc->imm_u16;
	addr |= (((uint32_t)
		(bits(idc->instr, 8, 4) >> 3) | bits(idc->instr, 0, 0)) << 16);

	pc2 = pc_start + 2;
	pushbyte(pc2 & 0xff);
	pushbyte((pc2 >> 8) & 0xff);
	if (pc22)
		pushbyte(pc2 >> 16);

	pc = addr - instr_size;
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
instr_mul(struct instr_decode_common *idc)
{
	uint16_t res;

	res = (uint16_t)memory[idc->ddddd] * (uint16_t)memory[idc->rrrrr];
	memwriteword(0, res);

	mul_flags16(res, &idc->setflags, &idc->clrflags);
}

void
instr_muls(struct instr_decode_common *idc)
{
	uint16_t res;
	int16_t rd, rs;

	rd = (int8_t)memory[idc->ddddd];
	rs = (int8_t)memory[idc->rrrrr];

	res = rd * rs;
	memwriteword(0, res);

	mul_flags16(res, &idc->setflags, &idc->clrflags);
}

void
instr_mulsu(struct instr_decode_common *idc)
{
	uint16_t res;
	int16_t rd, rs;

	rd = (int8_t)memory[idc->ddddd];
	rs = (uint8_t)memory[idc->rrrrr];

	res = rd * rs;
	memwriteword(0, res);

	mul_flags16(res, &idc->setflags, &idc->clrflags);
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

	pushbyte(memory[idc->ddddd]);
}

void
instr_unimp(struct instr_decode_common *idc __unused)
{

	unhandled(idc->instr);
}
