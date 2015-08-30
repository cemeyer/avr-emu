#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "test.h"

/* Make it harder to forget to add tests to suite. */
#pragma GCC diagnostic error "-Wunused-function"

#define	TESTODIR	"testoutput/"

static void
setup(void)
{
}

static void
teardown(void)
{
}

START_TEST(test_nop_trace)
{
#define	TRACEF	"nop3.trace"
	FILE *t;
	char buf[40];
	size_t rd;
	int ign;

	ign = system("./avr-emu -l 3 -t " TESTODIR TRACEF
	    " -x testfiles/nop3.bin >/dev/null 2>/dev/null");
	(void)ign;

	t = fopen(TESTODIR TRACEF, "rb");
	ck_assert_ptr_ne(t, NULL);

	memset(buf, 0, sizeof(buf));
	rd = fread(buf, 1, sizeof(buf) - 1, t);
	ck_assert_uint_gt(rd, 0);

	ck_assert_str_eq(buf, "0000 \n0000 \n0000 \n");

	fclose(t);
#undef	TRACEF
}
END_TEST

Suite *
suite_emu(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("emu");

	t = tcase_create("trace");
	tcase_add_checked_fixture(t, setup, teardown);
	tcase_add_test(t, test_nop_trace);
	suite_add_tcase(s, t);

	return (s);
}
