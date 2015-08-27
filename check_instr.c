#include "emu.h"

#include <check.h>

#define	PC_START		0

/* Make it harder to forget to add tests to suite. */
#pragma GCC diagnostic error "-Wunused-function"

void
install_words(uint16_t *code, uint32_t addr, size_t sz)
{

	memcpy(&flash[addr], code, sz);
}

void
setup_machine(void)
{

	// zero regs/mem, clear symbols
	init();
}

void
setup_machine22(void)
{

	// zero regs/mem, clear symbols
	init();
	pc22 = true;
}

void
teardown_machine(void)
{

	destroy();
}

START_TEST(test_nop)
{
	uint16_t code[] = {
		0x0000,
		0x0000,
	};

	install_words(code, PC_START, sizeof(code));
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
}
END_TEST

START_TEST(test_mov)
{
	uint16_t code[] = {
		0x2c00 | /*r_rrrr*/ 0x20f | /*ddddd*/ 0x1e0,	/* mov r30,r31 */
		0x2c00 | /*r_rrrr*/ 0x20e | /*ddddd*/ 0x050,	/* mov r5,r30 */
		0x2c00 | /*r_rrrr*/ 0x005 | /*ddddd*/ 0x000,	/* mov r0,r5 */
	};

	install_words(code, PC_START, sizeof(code));
	memory[31] = 0xab;
	ck_assert_uint_eq(memory[30], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[30], 0xab);
	ck_assert_uint_eq(memory[5], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[5], 0xab);
	ck_assert_uint_eq(memory[0], 0);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[0], 0xab);
}
END_TEST

START_TEST(test_or)
{
	uint16_t code[] = {
		0x2800 | /*r_rrrr*/ 0x20f | /*ddddd*/ 0x1e0,	/* or r30,r31 */
		0x2800 | /*r_rrrr*/ 0x20d | /*ddddd*/ 0x050,	/* or r5,r29 */
		0x2800 | /*r_rrrr*/ 0x005 | /*ddddd*/ 0x000,	/* or r0,r5 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x0;
	memory[30] = 0x0;

	memory[29] = 0xaa;
	memory[5] = 0x55;

	memory[0] = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	// OR(0, 0) = 0
	ck_assert_uint_eq(memory[30], 0x0);
	ck_assert_uint_eq(memory[SREG], SREG_Z);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	// OR(0xaa, 0x55) = 0xff
	ck_assert_uint_eq(memory[5], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	// OR(0xff, 0xff) = 0xff
	ck_assert_uint_eq(memory[0], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);
}
END_TEST

START_TEST(test_ori)
{
	uint16_t code[] = {
		0x6000 | /*KKKK_KKKK*/ 0xf0f | /*dddd*/ 0x00,	/* ori r16, $0xff */
		0x6000 | /*KKKK_KKKK*/ 0x000 | /*dddd*/ 0x50,	/* ori r21, $0x0 */
		0x6000 | /*KKKK_KKKK*/ 0xa0a | /*dddd*/ 0xf0,	/* ori r31, $0xaa */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0x55;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	// OR(0, 0xff) = 0xff
	ck_assert_uint_eq(memory[16], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	// OR(0, 0) = 0
	ck_assert_uint_eq(memory[21], 0);
	ck_assert_uint_eq(memory[SREG], SREG_Z);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	// OR(0xaa, 0x55) = 0xff
	ck_assert_uint_eq(memory[31], 0xff);
	ck_assert_uint_eq(memory[SREG], SREG_N | SREG_S);
}
END_TEST

START_TEST(test_out)
{
	uint16_t code[] = {
		0xb800 | /*AA_AAAA*/ 0x60e | /*rrrrr*/ 0x1f0,	/* out $3e, r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[IO_BASE + 0x3e], 0xab);
}
END_TEST

START_TEST(test_in)
{
	uint16_t code[] = {
		0xb000 | /*AA_AAAA*/ 0x60e | /*rrrrr*/ 0x1f0,	/* in r31, $3e */
	};

	install_words(code, PC_START, sizeof(code));

	memory[IO_BASE + 0x3e] = 0xab;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0xab);
}
END_TEST

START_TEST(test_push)
{
	uint16_t code[] = {
		0x920f | /*rrrrr*/ 0x1f0,	/* push r31 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;

	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(getsp(), 0xfffe);
	ck_assert_uint_eq(memory[0xffff], 0xab);
}
END_TEST

START_TEST(test_movw)
{
	uint16_t code[] = {
		0x0100 | /*dddd*/ 0xe0 | /*rrrr*/ 0xf,	/* mov r29:28, r31:r30 */
	};

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xab;
	memory[30] = 0xcd;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[29], memory[31]);
	ck_assert_uint_eq(memory[28], memory[30]);
}
END_TEST

START_TEST(test_call)
{
	uint16_t code[] = {
		0x940e /*kkkkk_k = 0*/,		/* call 0xdead */
		0xdead
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0xdead);
	ck_assert_uint_eq(getsp(), 0xfffd);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], (PC_START + 2) >> 8);
}
END_TEST

START_TEST(test_call22)
{
	uint16_t code[] = {
		0x940e | /*kkkkk_k*/ 0x1f1,	/* call 0x3fbeef */
		0xbeef
	};

	install_words(code, PC_START, sizeof(code));
	setsp(0xffff);

	emulate1();
	ck_assert_uint_eq(pc, 0x3fbeef);
	ck_assert_uint_eq(getsp(), 0xfffc);
	ck_assert_uint_eq(memory[0xffff], (PC_START + 2) & 0xff);
	ck_assert_uint_eq(memory[0xfffe], ((PC_START + 2) >> 8) & 0xff);
	ck_assert_uint_eq(memory[0xfffd], (PC_START + 2) >> 16);
}
END_TEST

START_TEST(test_mul)
{
	uint16_t code[] = {
		0x9c00 | 0x3fe,		/* mul r31,r30 */
		0x9c00 | 0x001,		/* mul r0, r1 */
		0x9c00 | 0x045,		/* mul r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xff;
	memory[30] = 0xff;
	memory[4] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xff * 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0xfe * 0x01);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_muls)
{
	uint16_t code[] = {
		0x0200 | 0xfe,		/* muls r31,r30 */
		0x0200 | 0x45,		/* muls r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[31] = 0xff;
	memory[30] = 0x2;
	memory[20] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xfffe);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_mulsu)
{
	uint16_t code[] = {
		0x0300 | 0x76,		/* mulsu r23, r22 */
		0x0300 | 0x45,		/* mulsu r20, r21 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[23] = 0xff;
	memory[22] = 0xff;
	memory[20] = 0x15;
	memory[SREG] = sreg_start = 0xff & ~(SREG_Z | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memword(0), 0xff01);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C);

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memword(0), 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_adc)
{
	uint16_t code[] = {
		0x1c00 | 0x01,		/* adc r0, r1 */
		0x1c00 | 0x23,		/* adc r2, r3 */
		0x1c00 | 0x45,		/* adc r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[0] = 0xf0;
	memory[1] = 0x0f;
	sreg_start = SREG_I | SREG_T;
	memory[SREG] = sreg_start | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z | SREG_H);

	memory[2] = 0x40;
	memory[3] = 0x40;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[2], 0x81 /* Carry from previous instr */);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[4], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);
}
END_TEST

START_TEST(test_add)
{
	uint16_t code[] = {
		0x0c00 | 0x01,		/* add r0, r1 */
		0x0c00 | 0x23,		/* add r2, r3 */
		0x0c00 | 0x45,		/* add r4, r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[0] = 0xf1;
	memory[1] = 0x0f;
	memory[SREG] = sreg_start = SREG_I | SREG_T;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z | SREG_H);

	memory[2] = 0x40;
	memory[3] = 0x40;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[2], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[4], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);
}
END_TEST

START_TEST(test_adiw)
{
	uint16_t code[] = {
		0x9600 | 0x00 | 0x5,		/* addiw r25:24, $5 */
		0x9600 | 0x30 | 0xcf,		/* addiw r31:30, $3f */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[24] = 0xfb;
	memory[25] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[24], 0);
	ck_assert_uint_eq(memory[25], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_Z);

	memory[30] = 0xff;
	memory[31] = 0x7f;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[30], 0x3e);
	ck_assert_uint_eq(memory[31], 0x80);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_V | SREG_N);
}
END_TEST

START_TEST(test_and)
{
	uint16_t code[] = {
		0x2000 | 0x01,		/* and r0,r1 */
		0x2000 | 0x45,		/* and r4,r5 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));

	memory[1] = 0xff;
	memory[0] = 0xff;

	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[0], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	memory[4] = 0xff;
	memory[5] = 0;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[4], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_andi)
{
	uint16_t code[] = {
		0x7000 | 0x00 | 0xf0f,	/* andi r16, $ff */
		0x7000 | 0x40 | 0,	/* andi r20, $0 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));
	memory[16] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H | SREG_C;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[16], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_S);

	memory[20] = 0xff;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[20], 0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_Z);
}
END_TEST

START_TEST(test_asr)
{
	uint16_t code[] = {
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
		0x9405 | 0x1f0,		/* asr r31 */
	};
	uint8_t sreg_start;

	install_words(code, PC_START, sizeof(code));
	memory[31] = 0xff;
	memory[SREG] = sreg_start = SREG_I | SREG_T | SREG_H;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 1);
	ck_assert_uint_eq(memory[31], 0xff);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_C | SREG_S);

	memory[31] = 0x81;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 2);
	ck_assert_uint_eq(memory[31], 0xc0);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_N | SREG_C | SREG_S);

	memory[31] = 0x7f;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 3);
	ck_assert_uint_eq(memory[31], 0x3f);
	ck_assert_uint_eq(memory[SREG], sreg_start | SREG_C | SREG_V | SREG_S);

	memory[31] = 0x70;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 4);
	ck_assert_uint_eq(memory[31], 0x38);
	ck_assert_uint_eq(memory[SREG], sreg_start);

	memory[31] = 0x1;

	emulate1();
	ck_assert_uint_eq(pc, PC_START + 5);
	ck_assert_uint_eq(memory[31], 0);
	ck_assert_uint_eq(memory[SREG],
	    sreg_start | SREG_C | SREG_Z | SREG_V | SREG_S);
}
END_TEST

Suite *
suite_instr(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("instr");

	t = tcase_create("nop");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_nop);
	suite_add_tcase(s, t);

	t = tcase_create("basic");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_and);
	tcase_add_test(t, test_andi);
	tcase_add_test(t, test_call);
	tcase_add_test(t, test_in);
	tcase_add_test(t, test_mov);
	tcase_add_test(t, test_or);
	tcase_add_test(t, test_ori);
	tcase_add_test(t, test_out);
	tcase_add_test(t, test_push);
	suite_add_tcase(s, t);

	t = tcase_create("regpairs");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_movw);
	suite_add_tcase(s, t);

	t = tcase_create("22-bit pc");
	tcase_add_checked_fixture(t, setup_machine22, teardown_machine);
	tcase_add_test(t, test_call22);
	suite_add_tcase(s, t);

	t = tcase_create("math");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_adc);
	tcase_add_test(t, test_add);
	tcase_add_test(t, test_adiw);
	tcase_add_test(t, test_mul);
	tcase_add_test(t, test_muls);
	tcase_add_test(t, test_mulsu);
	suite_add_tcase(s, t);

	t = tcase_create("shifts");
	tcase_add_checked_fixture(t, setup_machine, teardown_machine);
	tcase_add_test(t, test_asr);
	suite_add_tcase(s, t);

	return s;
}

int
main(void)
{
	Suite *s = suite_instr();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	return srunner_ntests_failed(sr);
}
