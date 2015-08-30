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
fmul_flags16(uint16_t res, uint8_t *set, uint8_t *clr)
{

	if ((res & 0x8000) != 0)
		*set |= SREG_C;
	else
		*clr |= SREG_C;

	res <<= 1;
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
sub_flags8(uint8_t res, uint8_t rd, uint8_t rr, uint8_t *set, uint8_t *clr)
{

	if (res & 0x80)
		*set |= SREG_N;
	else
		*clr |= SREG_N;
	if ((0x80 & rd & ~rr & ~res) ||
	    (0x80 & ~rd & rr & res))
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

	if ((0x80 & ~rd & rr) ||
	    (0x80 & rr & res) ||
	    (0x80 & res & ~rd))
		*set |= SREG_C;
	else
		*clr |= SREG_C;
	if ((0x08 & ~rd & rr) ||
	    (0x08 & rr & res) ||
	    (0x08 & res & ~rd))
		*set |= SREG_H;
	else
		*clr |= SREG_H;
}

static inline uint8_t
popbyte(void)
{
	uint16_t sp;

	sp = getsp();
	setsp(sp + 1);
	return (memory[sp + 1]);
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
instr_bclrset(struct instr_decode_common *idc)
{
	uint8_t sss;
	bool clr;

	sss = idc->ddddd - 16;
	clr = (idc->instr & 0x80);

	if (clr)
		idc->clrflags |= (1 << sss);
	else
		idc->setflags |= (1 << sss);
}

void
instr_bld(struct instr_decode_common *idc)
{
	uint8_t sss;

	sss = idc->rrrrr - 16;
	if ((memory[SREG] & SREG_T) != 0)
		memory[idc->ddddd] |= (1 << sss);
	else
		memory[idc->ddddd] &= ~(1 << sss);
}

void
instr_brb(struct instr_decode_common *idc)
{
	uint8_t sss;
	int8_t imm_s8;
	bool clr;

	sss = idc->rrrrr - 16;
	clr = (idc->instr & 0x0400);
	imm_s8 = (bits(idc->instr, 9, 3) >> 3);
	if ((imm_s8 & 0x40) != 0)
		imm_s8 |= 0x80;

	if ((memory[SREG] & (1 << sss)) != 0 && !clr)
		pc += imm_s8;
	else if ((memory[SREG] & (1 << sss)) == 0 && clr)
		pc += imm_s8;
}

void
instr_bst(struct instr_decode_common *idc)
{
	uint8_t sss;

	sss = idc->rrrrr - 16;
	if ((memory[idc->ddddd] & (1 << sss)) != 0)
		idc->setflags |= SREG_T;
	else
		idc->clrflags |= SREG_T;
}

void
instr_call(struct instr_decode_common *idc)
{
	uint32_t addr, pc2;

	addr = idc->imm_u16;
	if (pc22)
		addr |= (((uint32_t)
			(bits(idc->instr, 8, 4) >> 3) |
			bits(idc->instr, 0, 0)) << 16);

	pc2 = pc_start + 2;
	pushbyte(pc2 & 0xff);
	pushbyte((pc2 >> 8) & 0xff);
	if (pc22)
		pushbyte(pc2 >> 16);

	pc = addr - instr_size;
}

void
instr_cbisbi(struct instr_decode_common *idc)
{
	bool set;
	uint8_t sss, A;

	sss = idc->rrrrr - 16;
	set = (bits(idc->instr, 9, 9) != 0);

	A = bits(idc->instr, 7, 3) >> 3;
	if (set)
		memory[IO_BASE + A] |= (1 << sss);
	else
		memory[IO_BASE + A] &= ~(1 << sss);
}

void
instr_com(struct instr_decode_common *idc)
{
	uint8_t res, *set, *clr;

	res = (0xff - memory[idc->ddddd]);
	memory[idc->ddddd] = res;

	set = &idc->setflags;
	clr = &idc->clrflags;

	*set |= SREG_C;
	*clr |= SREG_V;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;
	if ((res & 0x80) != 0)
		*set |= (SREG_N | SREG_S);
	else
		*clr |= (SREG_N | SREG_S);
}

/* CP, CPC, SUB, SBC */
void
instr_cpc(struct instr_decode_common *idc)
{
	bool carry_mode, nostore;
	uint8_t rr, rd, res;

	carry_mode = (bits(idc->instr, 12, 12) == 0);
	nostore = ((bits(idc->instr, 11, 10) >> 10) == 1);

	rr = memory[idc->rrrrr];
	rd = memory[idc->ddddd];

	res = rd - rr;
	if (carry_mode && (memory[SREG] & SREG_C) != 0)
		res -= 1;
	if (!nostore)
		memory[idc->ddddd] = res;

	sub_flags8(res, rd, rr, &idc->setflags, &idc->clrflags);
	idc->setflags &= ~SREG_Z;
}

/* CPI, SBCI, SUBI */
void
instr_cpi(struct instr_decode_common *idc)
{
	uint8_t rd, res;
	bool store, carry;

	store = ((bits(idc->instr, 14, 13) >> 13) == 2);
	carry = (bits(idc->instr, 12, 12) == 0);

	rd = memory[idc->ddddd];
	res = rd - idc->imm_u8;
	if (carry && (memory[SREG] & SREG_C) != 0)
		res--;
	if (store)
		memory[idc->ddddd] = res;

	sub_flags8(res, rd, idc->imm_u8, &idc->setflags, &idc->clrflags);
}

void
instr_cpse(struct instr_decode_common *idc)
{

	if (memory[idc->ddddd] == memory[idc->rrrrr])
		skip_next_instruction = true;
}

void
instr_dec(struct instr_decode_common *idc)
{
	uint8_t rd, res;

	res = memory[idc->ddddd] - 1;
	memory[idc->ddddd] = res;

	logic_flags8(res, &idc->setflags, &idc->clrflags);
	if (res == 0x7f) {
		idc->clrflags &= ~(SREG_V | SREG_S);
		idc->setflags |= (SREG_V | SREG_S);
	}
}

void
instr_eicalljump(struct instr_decode_common *idc)
{
	uint32_t pc2, addr;
	bool ext, call;

	addr = memword(REGP_Z);
	ext = bits(idc->instr, 4, 4);
	call = bits(idc->instr, 8, 8);

	if (ext && pc22)
		addr |= ((uint32_t)memory[EIND] << 16);
	else if (ext)
		illins(idc->instr);

	if (call) {
		pc2 = pc_start + 1;
		pushbyte(pc2 & 0xff);
		pushbyte((pc2 >> 8) & 0xff);
		if (pc22)
			pushbyte(pc2 >> 16);
	}

	pc = addr - instr_size;
}

void
instr_elpm(struct instr_decode_common *idc)
{
	uint32_t addr;
	uint16_t word;
	bool ext;

	ext = bits(idc->instr, 4, 4);
	addr = memword(REGP_Z);

	if (ext)
		addr |= ((uint32_t)memory[RAMPZ] << 16);

	word = romword(addr >> 1);
	if ((addr & 1) != 0)
		memory[0] = (word >> 8);
	else
		memory[0] = (word & 0xff);
}

void
instr_elpmz(struct instr_decode_common *idc)
{
	uint32_t addr;
	uint16_t word;
	bool ext, postinc;

	ext = bits(idc->instr, 1, 1);
	postinc = bits(idc->instr, 0, 0);

	if (postinc && (idc->ddddd == 30 || idc->ddddd == 31)) {
		printf("%s: ELPM r30, Z+ and ELPM r31, Z+ are documented as "
		    "undefined in the AVR Instruction set manual.\n",
		    __func__);
		illins(idc->instr);
	}

	addr = memword(REGP_Z);
	if (ext)
		addr |= ((uint32_t)memory[RAMPZ] << 16);

	word = romword(addr >> 1);
	if ((addr & 1) != 0)
		memory[idc->ddddd] = (word >> 8);
	else
		memory[idc->ddddd] = (word & 0xff);

	if (postinc) {
		addr++;
		memwriteword(REGP_Z, addr & 0xffff);
	}
}

void
instr_fmul(struct instr_decode_common *idc)
{
	uint16_t res;

	res = (uint16_t)memory[idc->ddddd] * (uint16_t)memory[idc->rrrrr];

	fmul_flags16(res, &idc->setflags, &idc->clrflags);

	res <<= 1;
	memwriteword(0, res);
}

void
instr_fmulsu(struct instr_decode_common *idc)
{
	int16_t rd, rr;
	uint16_t res;
	bool su;

	su = bits(idc->instr, 3, 3);

	rd = (int8_t)memory[idc->ddddd];
	if (su)
		rr = (uint8_t)memory[idc->rrrrr];
	else
		rr = (int8_t)memory[idc->rrrrr];
	res = rd * rr;

	fmul_flags16(res, &idc->setflags, &idc->clrflags);

	res <<= 1;
	memwriteword(0, res);
}

void
instr_in(struct instr_decode_common *idc)
{
	uint8_t io_port;

	io_port = (bits(idc->instr, 10, 9) >> 5) | bits(idc->instr, 3, 0);
	memory[idc->ddddd] = memory[IO_BASE + io_port];
}

void
instr_inc(struct instr_decode_common *idc)
{
	uint8_t rd, res;

	res = memory[idc->ddddd] + 1;
	memory[idc->ddddd] = res;

	logic_flags8(res, &idc->setflags, &idc->clrflags);
	if (res == 0x80) {
		idc->clrflags &= ~SREG_V;
		idc->clrflags |= SREG_S;
		idc->setflags &= ~SREG_S;
		idc->setflags |= SREG_V;
	}
}

void
instr_jmp(struct instr_decode_common *idc)
{
	uint32_t addr;

	addr = idc->imm_u16;
	if (pc22)
		addr |= (((uint32_t)
			(bits(idc->instr, 8, 4) >> 3) |
			bits(idc->instr, 0, 0)) << 16);

	pc = addr - instr_size;
}

void
instr_ldi(struct instr_decode_common *idc)
{

	memory[idc->ddddd] = idc->imm_u8;
}

void
instr_lds(struct instr_decode_common *idc)
{
	uint32_t addr, pc2;

	addr = idc->imm_u16;
	if (!pc_mem_max_64k)
		addr |= ((uint32_t)memory[RAMPD] << 16);

	memory[idc->ddddd] = memory[addr];
}

/*
 * mode:
 *   0: with displacement
 *   1: postinc
 *   2: predec
 *
 * displace:
 *   Always zero for X LD modes, but can be +[0, 63] for Y/Z modes.
 */
static inline void
instr_ldst_common(uint8_t rp, uint8_t extaddr, uint8_t mode, uint8_t displace,
    const char *name, bool store, struct instr_decode_common *idc)
{
	uint32_t addr;
	bool predec, postinc;

	predec = postinc = false;
	switch (mode) {
	case 1:
		postinc = true;
		break;
	case 2:
		predec = true;
		break;
	default:
		break;
	}

	if ((predec || postinc) && (idc->ddddd == rp || idc->ddddd == rp + 1)) {
		printf("%s: LD r%u, %s+/- and LD r%u, %s+/- are documented "
		    "as undefined in the AVR Instruction set manual.\n",
		    __func__, (uns)rp, name, (uns)rp + 1, name);
		illins(idc->instr);
	}

	addr = memword(rp);
	if (!pc_mem_max_64k)
		addr |= ((uint32_t)memory[extaddr] << 16);

	if (predec)
		addr = (addr - 1) & 0xffffff;

	if (!predec && !postinc)
		addr = (addr + displace) & 0xffffff;

	if (pc_mem_max_256b)
		addr &= 0xff;
	else if (pc_mem_max_64k)
		addr &= 0xffff;

	if (store)
		memory[addr] = memory[idc->ddddd];
	else
		memory[idc->ddddd] = memory[addr];

	if (postinc)
		addr++;

	if (postinc || predec) {
		if (pc_mem_max_256b)
			memory[rp] = (addr & 0xff);
		else {
			memwriteword(rp, addr & 0xffff);
			if (!pc_mem_max_64k)
				memory[extaddr] = (addr >> 16);
		}
	}
}

void
instr_ldx(struct instr_decode_common *idc)
{

	instr_ldst_common(REGP_X, RAMPX, bits(idc->instr, 1, 0), 0, "X", false,
	    idc);
}

void
instr_ldyz(struct instr_decode_common *idc)
{
	uint8_t displace, mode;
	bool Y;

	Y = bits(idc->instr, 3, 3);

	if (bits(idc->instr, 12, 12) == 0) {
		mode = 0;
		displace = (bits(idc->instr, 13, 13) >> 8) |
		    (bits(idc->instr, 11, 10) >> 7) |
		    bits(idc->instr, 2, 0);
	} else {
		mode = bits(idc->instr, 1, 0);
		displace = 0;
	}

	if (Y)
		instr_ldst_common(REGP_Y, RAMPY, mode, displace, "Y", false,
		    idc);
	else
		instr_ldst_common(REGP_Z, RAMPZ, mode, displace, "Z", false,
		    idc);
}

void
instr_lsr(struct instr_decode_common *idc)
{
	uint8_t rd, res;

	rd = memory[idc->ddddd];
	res = (rd >> 1);
	memory[idc->ddddd] = res;

	asr_flags8(res, rd, &idc->setflags, &idc->clrflags);
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
instr_neg(struct instr_decode_common *idc)
{
	uint8_t rd, res, *set, *clr;

	rd = memory[idc->ddddd];
	res = (0u - rd) & 0xff;
	memory[idc->ddddd] = res;

	set = &idc->setflags;
	clr = &idc->clrflags;

	if (res == 0) {
		*set |= SREG_Z;
		*clr |= SREG_C;
	} else {
		*clr |= SREG_Z;
		*set |= SREG_C;
	}
	if ((res & 0x80) != 0)
		*set |= SREG_N;
	else
		*clr |= SREG_N;

	if (res == 0x80)
		*set |= SREG_V;
	else
		*clr |= SREG_V;

	if ((res & 0x08) || (rd & 0x08))
		*set |= SREG_H;
	else
		*clr |= SREG_H;

	if (((*set & SREG_N) && (*clr & SREG_V)) ||
	    ((*clr & SREG_N) && (*set & SREG_V)))
		*set |= SREG_S;
	else
		*clr |= SREG_S;
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
instr_pop(struct instr_decode_common *idc)
{

	memory[idc->ddddd] = popbyte();
}

void
instr_push(struct instr_decode_common *idc)
{

	pushbyte(memory[idc->ddddd]);
}

void
instr_rcalljmp(struct instr_decode_common *idc)
{
	uint32_t pc2;
	int16_t k;
	bool call;

	call = bits(idc->instr, 12, 12);
	k = bits(idc->instr, 11, 0);
	if ((k & 0x800) != 0)
		k |= 0xf000;

	if (call) {
		pc2 = pc_start + 1;
		pushbyte(pc2 & 0xff);
		pushbyte((pc2 >> 8) & 0xff);
		if (pc22)
			pushbyte(pc2 >> 16);
	}

	pc = pc + k;
	/* Implicit +1 from instr_size. */
}

static inline void
instr_ret_common(void)
{
	uint32_t addr;

	addr = ((uint32_t)popbyte() << 8);
	addr |= popbyte();
	if (pc22) {
		addr <<= 8;
		addr |= popbyte();
		addr &= 0x3fffff;
	}
	pc = addr - instr_size;
}

void
instr_ret(struct instr_decode_common *idc __unused)
{

	instr_ret_common();
}

void
instr_reti(struct instr_decode_common *idc __unused)
{

	instr_ret_common();
	memory[SREG] |= SREG_I;
}

void
instr_ror(struct instr_decode_common *idc)
{
	uint8_t rd, res;

	rd = memory[idc->ddddd];
	res = (rd >> 1);
	if ((memory[SREG] & SREG_C) != 0)
		res |= 0x80;
	memory[idc->ddddd] = res;

	asr_flags8(res, rd, &idc->setflags, &idc->clrflags);
}

void
instr_sbics(struct instr_decode_common *idc)
{
	uint8_t b, A;
	bool ifset, bit;

	ifset = (bits(idc->instr, 9, 9) != 0);

	b = idc->rrrrr - 16;
	A = (bits(idc->instr, 7, 3) >> 3);

	bit = (memory[IO_BASE + A] & (1 << b));

	if (ifset == bit)
		skip_next_instruction = true;
}

void
instr_sbiw(struct instr_decode_common *idc)
{
	uint16_t res, rd;
	uint8_t K, d, *set, *clr;

	d = (bits(idc->instr, 5, 4) >> 3) + 24;
	K = (bits(idc->instr, 7, 6) >> 2) | bits(idc->instr, 3, 0);

	rd = memword(d);
	res = rd - K;
	memwriteword(d, res);

	set = &idc->setflags;
	clr = &idc->clrflags;

	if (res == 0)
		*set |= SREG_Z;
	else
		*clr |= SREG_Z;
	if (0x8000 & res & ~rd)
		*set |= SREG_C;
	else
		*clr |= SREG_C;
	if (0x8000 & res)
		*set |= SREG_N;
	else
		*clr |= SREG_N;
	if (0x8000 & rd & ~res)
		*set |= SREG_V;
	else
		*clr |= SREG_V;

	if (((*set & SREG_N) && (*clr & SREG_V)) ||
	    ((*clr & SREG_N) && (*set & SREG_V)))
		*set |= SREG_S;
	else
		*clr |= SREG_S;
}

void
instr_sbrcs(struct instr_decode_common *idc)
{
	uint8_t b;
	bool ifset, bit;

	ifset = (bits(idc->instr, 9, 9) != 0);

	b = idc->rrrrr - 16;
	bit = (memory[idc->ddddd] & (1 << b));

	if (ifset == bit)
		skip_next_instruction = true;
}

void
instr_stx(struct instr_decode_common *idc)
{

	instr_ldst_common(REGP_X, RAMPX, bits(idc->instr, 1, 0), 0, "X", true, idc);
}

void
instr_styz(struct instr_decode_common *idc)
{
	uint8_t displace, mode;
	bool Y;

	Y = bits(idc->instr, 3, 3);

	if (bits(idc->instr, 12, 12) == 0) {
		mode = 0;
		displace = (bits(idc->instr, 13, 13) >> 8) |
		    (bits(idc->instr, 11, 10) >> 7) |
		    bits(idc->instr, 2, 0);
	} else {
		mode = bits(idc->instr, 1, 0);
		displace = 0;
	}

	if (Y)
		instr_ldst_common(REGP_Y, RAMPY, mode, displace, "Y", true,
		    idc);
	else
		instr_ldst_common(REGP_Z, RAMPZ, mode, displace, "Z", true,
		    idc);
}

void
instr_xor(struct instr_decode_common *idc)
{

	memory[idc->ddddd] ^= memory[idc->rrrrr];
	logic_flags8(memory[idc->ddddd], &idc->setflags, &idc->clrflags);
}

void
instr_unimp(struct instr_decode_common *idc __unused)
{

	unhandled(idc->instr);
}
