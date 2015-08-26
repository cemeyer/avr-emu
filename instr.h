#ifndef	__INSTR_H__
#define	__INSTR_H__

struct instr_decode_common {
	uint8_t		ddddd;
	uint8_t		rrrrr;
	uint8_t		imm_u8;



	uint8_t		setflags;
	uint8_t		clrflags;
};

struct instr_decode {
	uint16_t	  pattern;
	uint16_t	  mask;

	void		(*code)(struct instr_decode_common *);

	bool		  ddddd84 : 1;
	bool		  dddd74 : 1;
	bool		  rrrrr9_30 : 1;
	bool		  KKKK118_30 : 1;
};

void instr_mov(struct instr_decode_common *);
void instr_nop(struct instr_decode_common *);
void instr_or(struct instr_decode_common *);
void instr_ori(struct instr_decode_common *);

#endif
