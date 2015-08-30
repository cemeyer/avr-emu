#include <check.h>

#include "test.h"

static Suite *(*suites[])(void) = {
	suite_emu,
	suite_instr,
	NULL,
};

int
main(void)
{
	SRunner *sr;
	Suite *s;
	unsigned i;
	int nfailed;

	nfailed = 0;

	for (i = 0; suites[i] != NULL; i++) {
		s = suites[i]();
		sr = srunner_create(s);
		srunner_run_all(sr, CK_NORMAL);
		nfailed += srunner_ntests_failed(sr);
	}

	return (nfailed);
}
