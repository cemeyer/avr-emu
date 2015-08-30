#include <unistd.h>

#include "emu.h"
#include "instr.h"

struct inprec {
	uint64_t	 ir_insn;
	size_t		 ir_len;
	char		 ir_inp[0];
};

/* Machine state */
uint32_t	 pc,
		 pc_start,
		 instr_size;
bool		 skip_next_instruction;
/* 24 bits are addressable in both Data and Program regions */
uint8_t		 memory[0x1000000];
/* Program memory is word-addressed */
uint16_t	 flash[ 0x1000000 / sizeof(uint16_t)];

/* Machine model characteristics */
bool		 pc22;
bool		 pc_mem_max_64k;
bool		 pc_mem_max_256b;

/* Emulater / GDB auxiliary info */
uint64_t	 start;		/* Start time in us */
uint64_t	 insns;
uint64_t	 insnlimit;
uint64_t	 insnreplaylim;
bool		 off;
bool		 replay_mode;
bool		 ctrlc;

bool		 tracehex;
FILE		*tracefile;

GHashTable	*input_record;			// insns -> inprec

// Could easily sort by popularity over time.
static struct instr_decode avr_instr[] = {
	{ 0x0000, 0xffff, instr_nop },
	{ 0x0100, 0xff00, instr_movw, .dddd74 = true, .rrrr30 = true },
	{ 0x0200, 0xff00, instr_muls, .dddd74 = true, .rrrr30 = true },
	{ 0x0300, 0xff88, instr_mulsu, .ddd64 = true, .rrr20 = true },
	{ 0x0308, 0xff88, instr_fmul, .ddd64 = true, .rrr20 = true },
	{ 0x0380, 0xff80, instr_fmulsu, .ddd64 = true, .rrr20 = true },
	{ 0x0400, 0xec00, instr_cpc, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x0800, 0xec00, instr_cpc, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x0c00, 0xec00, instr_adc, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x1000, 0xfc00, instr_cpse, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x2000, 0xfc00, instr_and, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x2400, 0xfc00, instr_xor, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x2800, 0xfc00, instr_or, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x2c00, 0xfc00, instr_mov, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0x3000, 0xf000, instr_cpi, .dddd74 = true, .KKKK118_30 = true },
	{ 0x4000, 0xe000, instr_cpi, .dddd74 = true, .KKKK118_30 = true },
	{ 0x6000, 0xf000, instr_ori, .dddd74 = true, .KKKK118_30 = true },
	{ 0x7000, 0xf000, instr_andi, .dddd74 = true, .KKKK118_30 = true },
	{ 0x8000, 0xd200, instr_ldyz, .ddddd84 = true },
	{ 0x8200, 0xd200, instr_unimp/*STD*/, .ddddd84 = true },
	{ 0x9000, 0xfe0f, instr_lds, .ddddd84 = true, .imm16 = true },
	{ 0x9001, 0xfe07, instr_ldyz, .ddddd84 = true },
	{ 0x9002, 0xfe07, instr_ldyz, .ddddd84 = true },
	{ 0x9004, 0xfe0c, instr_elpmz, .ddddd84 = true },
	{ 0x900c, 0xfe0e, instr_ldx, .ddddd84 = true },
	{ 0x900e, 0xfe0f, instr_ldx, .ddddd84 = true },
	{ 0x900f, 0xfe0f, instr_pop, .ddddd84 = true },
	{ 0x9200, 0xfe0f, instr_unimp/*STS*/, .ddddd84 = true, .imm16 = true },
	{ 0x9201, 0xfe07, instr_unimp/*ST Y+/Z+*/, .ddddd84 = true },
	{ 0x9202, 0xfe07, instr_unimp/*LD -Y/-Z*/, .ddddd84 = true },
	{ 0x9204, 0xfe0f, instr_unimp/*XCH*/, .ddddd84 = true },
	{ 0x9205, 0xfe0f, instr_unimp/*LAS*/, .ddddd84 = true },
	{ 0x9206, 0xfe0f, instr_unimp/*LAC*/, .ddddd84 = true },
	{ 0x9207, 0xfe0f, instr_unimp/*LAT*/, .ddddd84 = true },
	{ 0x920c, 0xfe0f, instr_unimp/*ST X*/, .ddddd84 = true },
	{ 0x920d, 0xfe0f, instr_unimp/*ST X+*/, .ddddd84 = true },
	{ 0x920e, 0xfe0f, instr_unimp/*ST -X*/, .ddddd84 = true },
	{ 0x920f, 0xfe0f, instr_push, .ddddd84 = true },
	{ 0x9400, 0xfe0f, instr_com, .ddddd84 = true },
	{ 0x9401, 0xfe0f, instr_neg, .ddddd84 = true },
	{ 0x9402, 0xfe0f, instr_unimp/*SWAP*/, .ddddd84 = true },
	{ 0x9403, 0xfe0f, instr_inc, .ddddd84 = true },
	{ 0x9405, 0xfe0f, instr_asr, .ddddd84 = true },
	{ 0x9406, 0xfe0f, instr_lsr, .ddddd84 = true },
	{ 0x9407, 0xfe0f, instr_ror, .ddddd84 = true },
	{ 0x9408, 0xff0f, instr_bclrset, .ddd64 = true },
	{ 0x940a, 0xfe0f, instr_dec, .ddddd84 = true },
	{ 0x940b, 0xff0f, instr_unimp/*DES(k)*/, .dddd74 = true },
	{ 0x940c, 0xfe0e, instr_jmp, .imm16 = true },
	{ 0x940e, 0xfe0e, instr_call, .imm16 = true },
	{ 0x9508, 0xffff, instr_ret },
	{ 0x9409, 0xfeef, instr_eicalljump },
	{ 0x9518, 0xffff, instr_reti },
	{ 0x9588, 0xffff, instr_unimp/*SLEEP*/ },
	{ 0x9598, 0xffff, instr_unimp/*BREAK*/ },
	{ 0x95c8, 0xffef, instr_elpm },
	{ 0x95e8, 0xffff, instr_unimp/*SPM*/ },
	{ 0x95f8, 0xffff, instr_unimp/*SPM Z+*/ },
	{ 0x9600, 0xff00, instr_adiw },
	{ 0x9700, 0xff00, instr_sbiw },
	{ 0x9800, 0xfd00, instr_cbisbi, .rrr20 = true },
	{ 0x9900, 0xfd00, instr_sbics, .rrr20 = true },
	{ 0x9c00, 0xfc00, instr_mul, .ddddd84 = true, .rrrrr9_30 = true },
	{ 0xb000, 0xf800, instr_in, .ddddd84 = true },
	{ 0xb800, 0xf800, instr_out, .ddddd84 = true },
	{ 0xc000, 0xe000, instr_rcalljmp },
	{ 0xe000, 0xf000, instr_ldi, .dddd74 = true, .KKKK118_30 = true },
	{ 0xf000, 0xf800, instr_brb, .rrr20 = true },
	{ 0xf800, 0xfe08, instr_bld, .ddddd84 = true, .rrr20 = true },
	{ 0xfa00, 0xfe08, instr_bst, .ddddd84 = true, .rrr20 = true },
	{ 0xfc00, 0xfc08, instr_sbrcs, .ddddd84 = true, .rrr20 = true },
};

void
print_ips(void)
{
	uint64_t end = now();

	if (end == start)
		end++;

	printf("Approx. %ju instructions per second (Total: %ju).\n",
	    (uintmax_t)insns * 1000000 / (end - start), (uintmax_t)insns);
}

void
init(void)
{

	pc = 0;
	pc22 = false;
	pc_mem_max_256b = false;
	pc_mem_max_64k = false;
	insns = 0;
	off = false;
	skip_next_instruction = false;
	start = now();
	//memset(memory, 0, sizeof(memory));
}

void
destroy(void)
{
}

#ifndef EMU_CHECK
static void
ctrlc_handler(int s)
{

	(void)s;
	ctrlc = true;
}

void
usage(void)
{
	printf("usage: avr-emu FLAGS [binaryimage]\n"
		"\n"
		"  FLAGS:\n"
		"    -g            Debug with GDB\n"
		"    -t=TRACEFILE  Emit instruction trace\n"
		"    -x            Trace output in hex\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	size_t rd, idx;
	const char *romfname;
	FILE *romfile;
	int opt;
	bool waitgdb = false;

	if (argc < 2)
		usage();

	while ((opt = getopt(argc, argv, "gt:x")) != -1) {
		switch (opt) {
		case 'g':
			waitgdb = true;
			break;
		case 't':
			tracefile = fopen(optarg, "wb");
			if (!tracefile) {
				printf("Failed to open tracefile `%s'\n",
				    optarg);
				exit(1);
			}
			break;
		case 'x':
			tracehex = true;
			break;
		default:
			usage();
			break;
		}
	}

	if (optind >= argc)
		usage();

	romfname = argv[optind];

	romfile = fopen(romfname, "rb");
	ASSERT(romfile, "fopen");

	input_record = g_hash_table_new_full(NULL, NULL, NULL, free);
	ASSERT(input_record, "x");

	init();

	idx = 0;
	while (true) {
		rd = fread(&flash[idx], sizeof(flash[0]),
		    ARRAYLEN(flash) - idx, romfile);
		if (rd == 0)
			break;
		idx += rd;
	}
	printf("Loaded %zu words from image.\n", idx);

	fclose(romfile);
	signal(SIGINT, ctrlc_handler);

	pc = 0;
	if (waitgdb)
		gdbstub_init();

	emulate();

	printf("Got CPUOFF, stopped.\n");
	gdbstub_stopped();

	print_regs();
	print_ips();

	if (tracefile)
		fclose(tracefile);

	return 0;
}
#endif

void
emulate1(void)
{
	struct instr_decode_common idc;
	uint16_t instr;
	size_t i;

restart:
	pc_start = pc;
	instr_size = 1;

	instr = romword(pc);

	for (i = 0; i < ARRAYLEN(avr_instr); i++)
		if ((instr & avr_instr[i].mask) == avr_instr[i].pattern)
			break;

	if (i == ARRAYLEN(avr_instr))
		illins(instr);

	memset(&idc, 0, sizeof(idc));
	idc.instr = instr;

	if (avr_instr[i].imm16) {
		idc.imm_u16 = romword(pc + 1);
		instr_size++;
	}

	/*
	 * After our CPSE, we've done enough decoding to figure out if this is
	 * a 16-bit or 32-bit instruction.
	 */
	if (skip_next_instruction) {
		skip_next_instruction = false;
		pc += instr_size;
		return;
	}

	if (avr_instr[i].ddddd84)
		idc.ddddd = bits(instr, 8, 4) >> 4;
	if (avr_instr[i].dddd74)
		idc.ddddd = 16 + (bits(instr, 7, 4) >> 4);
	if (avr_instr[i].ddd64)
		idc.ddddd = 16 + (bits(instr, 6, 4) >> 4);

	if (avr_instr[i].rrrrr9_30)
		idc.rrrrr = (bits(instr, 9, 9) >> 5) | bits(instr, 3, 0);
	if (avr_instr[i].rrrr30)
		idc.rrrrr = 16 + bits(instr, 3, 0);
	if (avr_instr[i].rrr20)
		idc.rrrrr = 16 + bits(instr, 2, 0);

	if (avr_instr[i].KKKK118_30)
		idc.imm_u8 = (bits(instr, 11, 8) >> 4) | bits(instr, 3, 0);

	avr_instr[i].code(&idc);

#ifndef	REALLYFAST
	ASSERT((idc.clrflags & idc.setflags) == 0, "overlapped flags: 0x%02x",
	    (unsigned)(idc.clrflags & idc.setflags));
#endif
	memory[SREG] &= ~idc.clrflags;
	memory[SREG] |= idc.setflags;

	pc += instr_size;

	if (!replay_mode && tracefile) {
		ASSERT(instr_size > 0 && instr_size < 3, "instr_size: %u",
		    (uns)instr_size);

		for (i = 0; i < instr_size; i++) {
			uint16_t word;

			word = romword(pc_start + i);
			if (tracehex)
				fprintf(tracefile, "%04x ", (uns)word);
			else {
				size_t wr;
				wr = fwrite(&word, 2, 1, tracefile);
				ASSERT(wr == 1, "fwrite: %s", strerror(errno));
			}
		}
		if (tracehex)
			fprintf(tracefile, "\n");
	}

	insns++;

	/*
	 * We need to do instruction decode on the *next* instruction to figure
	 * out where PC should be after *this* instruction.
	 */
	if (skip_next_instruction)
		goto restart;
}

static void
dumpmem(uint16_t addr, unsigned len)
{

	for (unsigned i = 0; i < len; i++) {
		printf("%02x", membyte(addr+i));
		if (i % 0x10 == 0xf)
			printf("\n");
	}
}

void
emulate(void)
{

#ifndef QUIET
	printf("Initial register state:\n");
	print_regs();
	printf("============================================\n\n");
#endif

	while (true) {
		if (ctrlc) {
			printf("Got ^C, stopping...\n");
			abort_nodump();
		}

#ifndef EMU_CHECK
		if (replay_mode && insns >= insnreplaylim) {
			replay_mode = false;
			insnreplaylim = 0;
			// return control to remote GDB
			stepone = true;
		}

		if (!replay_mode)
			gdbstub_intr();

		if (replay_mode && insnreplaylim < insns) {
			init();
			pc = 0;
			continue;
		}
#endif

		if (off)
			break;

		emulate1();

		if (off)
			break;

		if (insnlimit && insns >= insnlimit) {
			printf("\nXXX Hit insn limit, halting XXX\n");
			break;
		}
	}
}

void
_unhandled(const char *f, unsigned l, uint16_t instr)
{

	printf("%s:%u: Instruction: %#04x @PC=%#06x is not implemented\n",
	    f, l, (unsigned)instr, (unsigned)pc_start);
	printf("Raw at PC: ");
	for (unsigned i = 0; i < 3; i++)
		printf("%04x", flash[pc_start + i]);
	printf("\n");
	abort_nodump();
}

void
_illins(const char *f, unsigned l, uint16_t instr)
{

	printf("%s:%u: ILLEGAL Instruction: %#04x @PC=%#06x\n",
	    f, l, (unsigned)instr, (unsigned)pc_start);
	printf("Raw at PC: ");
	for (unsigned i = 0; i < 3; i++)
		printf("%04x", flash[pc_start + i]);
	printf("\n");
	abort_nodump();
}

void
abort_nodump(void)
{

	print_regs();
	print_ips();

#ifndef EMU_CHECK
	gdbstub_stopped();
#endif
	exit(1);
}

static void
printmemword(const char *pre, uint16_t addr)
{

	printf("%s", pre);
	printf("%02x", membyte(addr));
	printf("%02x", membyte(addr + 1));
}

#if 0
static void
printreg(unsigned reg)
{

	// XXX
	printf("%04x  ", registers[reg]);
}
#endif

void
print_regs(void)
{

#if 0
	// XXX
	printf("pc  ");
	printreg(PC);
	printf("sp  ");
	printreg(SP);
	printf("sr  ");
	printreg(SR);
	printf("cg  ");
	printreg(CG);
	printf("\n");

	for (unsigned i = 4; i < 16; i += 4) {
		for (unsigned j = i; j < i + 4; j++) {
			printf("r%02u ", j);
			printreg(j);
		}
		printf("\n");
	}

	printf("instr:");
	for (unsigned i = 0; i < 4; i++)
		printmemword("  ", (pc_start & 0xfffe) + 2*i);
	printf("\nstack:");
	for (unsigned i = 0; i < 4; i++)
		printmemword("  ", (registers[SP] & 0xfffe) + 2*i);
	printf("\n      ");
	for (unsigned i = 4; i < 8; i++)
		printmemword("  ", (registers[SP] & 0xfffe) + 2*i);
	printf("\n");
#endif
}

uint64_t
now(void)
{
	struct timespec ts;
	int rc;

	rc = clock_gettime(CLOCK_REALTIME, &ts);
	ASSERT(rc == 0, "clock_gettime: %d:%s", errno, strerror(errno));

	return ((uint64_t)sec * ts.tv_sec + (ts.tv_nsec / 1000));
}

#ifndef EMU_CHECK
static void
ins_inprec(char *dat, size_t sz)
{
	struct inprec *new_inp = malloc(sizeof *new_inp + sz + 1);

	ASSERT(new_inp, "oom");

	new_inp->ir_insn = insns;
	new_inp->ir_len = sz + 1;
	memcpy(new_inp->ir_inp, dat, sz);
	new_inp->ir_inp[sz] = 0;

	g_hash_table_insert(input_record, ptr(insns), new_inp);
}

void
getsn(uint16_t addr, uint16_t bufsz)
{
	struct inprec *prev_inp;
	char *buf;

	// XXX RAMEND or 24-bit at least
	ASSERT((size_t)addr + bufsz < 0xffff, "overflow");
	//memset(&memory[addr], 0, bufsz);

	if (bufsz <= 1)
		return;

	prev_inp = g_hash_table_lookup(input_record, ptr(insns));
	if (replay_mode)
		ASSERT(prev_inp, "input at insn:%ju not found!\n",
		    (uintmax_t)insns);

	if (prev_inp) {
		memcpy(&memory[addr], prev_inp->ir_inp, prev_inp->ir_len);
		return;
	}

	printf("Gets (':'-prefix for hex)> ");
	fflush(stdout);

	buf = malloc(2 * bufsz + 2);
	ASSERT(buf, "oom");
	buf[0] = 0;

	if (fgets(buf, 2 * bufsz + 2, stdin) == NULL)
		goto out;

	if (buf[0] != ':') {
		size_t len;

		len = strlen(buf);
		while (len > 0 &&
		    (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
			buf[len - 1] = '\0';
			len--;
		}

		strncpy((char*)&memory[addr], buf, bufsz);
		memory[addr + strlen(buf)] = 0;
		ins_inprec(buf, bufsz);
	} else {
		unsigned i;
		for (i = 0; i < bufsz - 1u; i++) {
			unsigned byte;

			if (buf[2*i+1] == 0 || buf[2*i+2] == 0) {
				memory[addr+i] = 0;
				break;
			}

			sscanf(&buf[2*i+1], "%02x", &byte);
			//printf("%02x", byte);
			memory[addr + i] = byte;
		}
		ins_inprec((void*)&memory[addr], i);
	}
out:
	free(buf);
}
#endif
