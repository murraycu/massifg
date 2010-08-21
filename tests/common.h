

#define TEST_INPUT_SHORT "massif-output-2snapshots.txt"
#define TEST_INPUT_LONG "massif-output-glom.txt"

/* Utility function for finding the test input files. Especially important for distcheck
 * http://www.gnu.org/software/automake/manual/automake.html#Tests
 * Caller should free return value using g_free () */
gchar *
get_test_file(const gchar *filename) {
	const gchar *srcdir = g_getenv("top_srcdir"); /* Should not be freed */
	gchar *path = g_build_filename(srcdir, "tests", filename, NULL);
	return path;
}
