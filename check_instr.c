#include "emu.h"

#include <check.h>

#define	PC_START		0
#define	ck_assert_flags(flags)	_ck_assert_flags(__LINE__, flags)

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
