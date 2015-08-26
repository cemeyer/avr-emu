#ifndef	__INSTR_H__
#define	__INSTR_H__

struct instr_decode {
	uint16_t	  pattern;
	uint16_t	  mask;

	void		(*code)(void);
};

void instr_nop(void);

#endif
