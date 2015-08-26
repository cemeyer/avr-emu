#include <unistd.h>

#include "emu.h"

struct inprec {
	uint64_t	 ir_insn;
	size_t		 ir_len;
	char		 ir_inp[0];
};

uint32_t	 pc,
		 pc_start,
		 instr_size;
/* 24 bits are addressable in both Data and Program regions */
uint8_t		 memory[0x1000000];
/* Program memory is word-addressed */
uint16_t	 flash[ 0x1000000 / sizeof(uint16_t)];
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

// Fast random numbers:
// 18:16 < rmmh> int k = 0x123456; int rand() { k=30903*(k&65535)+(k>>16);
//               return(k&65535); }

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

	insns = 0;
	off = false;
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
	uint16_t instr;

	pc_start = pc;
	instr_size = 1;

	instr = romword(pc);
	(void)instr;

	// XXX iterate instr table
	// if (match)
	//   if (match->dddd)
	//     dddd = XXX
	//   if (match->RRRR)
	//     RRRR = XXX
	//
	//   match->code ...

	pc += instr_size;

	if (!replay_mode && tracefile) {
		ASSERT(instr_size > 0 && instr_size < 3, "instr_size: %u",
		    (uns)instr_size);

		for (unsigned i = 0; i < instr_size; i++) {
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

#ifndef REALLYFAST
uint16_t
membyte(uint32_t addr)
{

	ASSERT((addr & ~0xffffff) == 0,
	    "load outside 24-bit addressable data memory: %#x",
	    (unsigned)addr);
	return (memory[addr]);
}

uint16_t
memword(uint32_t addr)
{

	ASSERT((addr & 0x1) == 0, "word load unaligned: %#04x",
	    (unsigned)addr);
	ASSERT((addr & ~0xffffff) == 0,
	    "load outside 24-bit addressable data memory: %#x",
	    (unsigned)addr);
	return (memory[addr + 1] | ((uint16_t)memory[addr] << 8));
}

void
memwriteword(uint32_t addr, uint16_t word)
{

	ASSERT((addr & 0x1) == 0, "word store unaligned: %#04x",
	    (uns)addr);
	memory[addr] = (word >> 8) & 0xff;
	memory[addr + 1] = word & 0xff;
}

/*
 * Flash is word-addressed (even though some operations select a single byte
 * from that word).  In particular PC addressed a word only.
 */
uint16_t
romword(uint32_t addr_word)
{

	ASSERT((addr_word & ~0x7fffff) == 0,
	    "load outside 24-bit addressable program memory: %#x",
	    (unsigned)addr_word);
	return (flash[addr_word]);
}
#endif

#ifndef REALLYFAST
uint16_t
bits(uint16_t v, unsigned max, unsigned min)
{
	uint16_t mask;

	ASSERT(max < 16 && max >= min, "bit-select");
	ASSERT(min < 16, "bit-select");

	mask = ((unsigned)1 << (max+1)) - 1;
	if (min > 0)
		mask &= ~( (1<<min) - 1 );

	return v & mask;
}
#endif

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
