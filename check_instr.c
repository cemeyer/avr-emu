#include "emu.h"

#include <check.h>

#define CODE_STEP   (0x4500)
#define CODE_REPEAT (0x4400)
#define PC_LOAD     (0xfffe)
#define CALL_GATE   (0x0010)

void
install_words_le(uint16_t *code, uint16_t addr, size_t sz)
{

	for (; sz > 1; sz -= 2) {
		uint16_t word = *code;

		memory[addr] = word & 0xff;
		memory[addr+1] = (word>>8) & 0xff;

		code++;
		addr += 2;
	}
}

void
setup_machine(void)
{
	uint16_t ret = 0x4130,
		 run = CODE_REPEAT;

	memset(memory, 0, sizeof(memory));

	// Setup callgate (ret)
	install_words_le(&ret, CALL_GATE, sizeof(ret));

	// Setup initial PC value @4400 (full emulation)
	install_words_le(&run, PC_LOAD, sizeof(run));

	// Setup intitial PC for single-step emu
	registers[PC] = CODE_STEP;

	init();
}

void
teardown_machine(void)
{

	destroy();
}

// mov #4400, sp
START_TEST(test_mov_const_reg)
{
	uint16_t code[] = {
		0x4031,
		0x4142,
	};

	install_words_le(code, CODE_STEP, sizeof(code));
	taint_mem(CODE_STEP + 2);

	emulate1();

	ck_assert(registers[PC] == CODE_STEP + 4);
	ck_assert(registers[SP] == 0x4142);
	ck_assert(regtaintedexcl(SP, CODE_STEP + 2));
}
END_TEST

// mov &#1000, r5
START_TEST(test_mov_sr_abs_reg)
{
	uint16_t code[] = {
		0x4215,
		0x1000,
	};
	uint16_t word = 0x1234;

	install_words_le(code, CODE_STEP, sizeof(code));
	install_words_le(&word, 0x1000, sizeof(word));
	taint_mem(0x1000);

	emulate1();

	ck_assert(registers[PC] == CODE_STEP + 4);
	ck_assert(registers[5] == 0x1234);
	ck_assert(regtaintedexcl(5, 0x1000));
}
END_TEST

// XXX TODO set flags (clear v, affect nzc)
// and.b #-1, r5
START_TEST(test_and_b_cgneg1_reg)
{
	uint16_t code[] = { 0xf375, };

	install_words_le(code, CODE_STEP, sizeof(code));
	registers[5] = 0x8182;

	emulate1();

	ck_assert(registers[PC] == CODE_STEP + 2);
	ck_assert(registers[5] == 0x0082);
}
END_TEST

START_TEST(test_bis_imm_reg)
{
	uint16_t code[] = {
		// mov #0082, r5
		0x4035,
		0x0082,
		// bis #0x5a08, r5
		0xd035,
		0x5a08,
	};

	install_words_le(code, CODE_STEP, sizeof(code));
	taint_mem(CODE_STEP + 2);
	taint_mem(CODE_STEP + 6);

	emulate1();
	emulate1();

	ck_assert(registers[PC] == CODE_STEP + 8);
	ck_assert(registers[5] == 0x5a8a);
	ck_assert(regtainted(5, CODE_STEP + 2));
	ck_assert(regtainted(5, CODE_STEP + 6));
}
END_TEST

Suite *
suite_instr(void)
{
	Suite *s = suite_create("instr");

	TCase *tmov = tcase_create("mov");
	tcase_add_checked_fixture(tmov, setup_machine, teardown_machine);
	tcase_add_test(tmov, test_mov_const_reg);
	tcase_add_test(tmov, test_mov_sr_abs_reg);
	suite_add_tcase(s, tmov);

	TCase *tand = tcase_create("and");
	tcase_add_checked_fixture(tand, setup_machine, teardown_machine);
	tcase_add_test(tand, test_and_b_cgneg1_reg);
	suite_add_tcase(s, tand);

	TCase *tbis = tcase_create("bis");
	tcase_add_checked_fixture(tbis, setup_machine, teardown_machine);
	tcase_add_test(tbis, test_bis_imm_reg);
	suite_add_tcase(s, tbis);

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