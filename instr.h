#ifndef	__INSTR_H__
#define	__INSTR_H__

struct instr_decode_common {
	uint16_t	instr;
	uint8_t		ddddd;
	uint8_t		rrrrr;
	uint8_t		imm_u8;
	uint16_t	imm_u16;

	uint8_t		setflags;
	uint8_t		clrflags;
};

struct instr_decode {
	uint16_t	  pattern;
	uint16_t	  mask;

	void		(*code)(struct instr_decode_common *);

	bool		  ddddd84 : 1;
	bool		  dddd74 : 1;
	bool		  ddd64 : 1;
	bool		  rrrrr9_30 : 1;
	bool		  rrrr30 : 1;
	bool		  rrr20 : 1;
	bool		  KKKK118_30 : 1;
	bool		  imm16 : 1;
};

void instr_adc(struct instr_decode_common *);
void instr_adiw(struct instr_decode_common *);
void instr_and(struct instr_decode_common *);
void instr_andi(struct instr_decode_common *);
void instr_asr(struct instr_decode_common *);
void instr_bclrset(struct instr_decode_common *);
void instr_bld(struct instr_decode_common *);
void instr_brb(struct instr_decode_common *);
void instr_bst(struct instr_decode_common *);
void instr_call(struct instr_decode_common *);
void instr_cbisbi(struct instr_decode_common *);
void instr_com(struct instr_decode_common *);
void instr_cpc(struct instr_decode_common *);
void instr_cpi(struct instr_decode_common *);
void instr_cpse(struct instr_decode_common *);
void instr_dec(struct instr_decode_common *);
void instr_eicalljump(struct instr_decode_common *);
void instr_elpm(struct instr_decode_common *);
void instr_elpmz(struct instr_decode_common *);
void instr_fmul(struct instr_decode_common *);
void instr_fmulsu(struct instr_decode_common *);
void instr_in(struct instr_decode_common *);
void instr_inc(struct instr_decode_common *);
void instr_jmp(struct instr_decode_common *);
void instr_ldi(struct instr_decode_common *);
void instr_lds(struct instr_decode_common *);
void instr_ldx(struct instr_decode_common *);
void instr_ldyz(struct instr_decode_common *);
void instr_lsr(struct instr_decode_common *);
void instr_mov(struct instr_decode_common *);
void instr_movw(struct instr_decode_common *);
void instr_mul(struct instr_decode_common *);
void instr_muls(struct instr_decode_common *);
void instr_mulsu(struct instr_decode_common *);
void instr_neg(struct instr_decode_common *);
void instr_nop(struct instr_decode_common *);
void instr_or(struct instr_decode_common *);
void instr_ori(struct instr_decode_common *);
void instr_out(struct instr_decode_common *);
void instr_pop(struct instr_decode_common *);
void instr_push(struct instr_decode_common *);
void instr_rcalljmp(struct instr_decode_common *);
void instr_ret(struct instr_decode_common *);
void instr_reti(struct instr_decode_common *);
void instr_ror(struct instr_decode_common *);
void instr_xor(struct instr_decode_common *);

void instr_unimp(struct instr_decode_common *);

#endif
