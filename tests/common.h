

/* Files that the test suite needs to access
 * A test-case checks that these exists and can be found */
#define TEST_INPUT_SHORT "massif-output-2snapshots.txt"
#define TEST_INPUT_LONG "massif-output-glom.txt"
#define TEST_INPUT_800 "massif-output-broken-800.txt"
#define TEST_INPUT_BOGUS "parser.c"

/* Utility function for finding the test input files. Especially important for distcheck
 * Caller should free return value using g_free () */
gchar *
get_test_file(const gchar *filename) {
	/* Autotools is responsible for setting this environment variable */
	const gchar *srcdir = g_getenv("top_srcdir"); /* Should not be freed */
	gchar *path = g_build_filename(srcdir, "tests", filename, NULL);
	return path;
}
