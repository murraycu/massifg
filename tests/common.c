
#include <glib.h>

#include "common.h"


/* Test that we can find our test input files */
void
locate_test_files(void) {
	gchar *str;

	str = get_test_file(TEST_INPUT_SHORT);
	g_assert(g_file_test(str, G_FILE_TEST_EXISTS));

	str = get_test_file(TEST_INPUT_LONG);
	g_assert(g_file_test(str, G_FILE_TEST_EXISTS));
}

int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/locate-test-files", locate_test_files);

	return g_test_run();
}
