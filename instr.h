#ifndef	__INSTR_H__
#define	__INSTR_H__

struct instr_decode_common {
	uint8_t		ddddd;
	uint8_t		rrrrr;
};

struct instr_decode {
	uint16_t	  pattern;
	uint16_t	  mask;

	void		(*code)(const struct instr_decode_common *);

	bool		  ddddd84 : 1;
	bool		  rrrrr9_30 : 1;
};

void instr_nop(const struct instr_decode_common *);
void instr_mov(const struct instr_decode_common *);

#endif
