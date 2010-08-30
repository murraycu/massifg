#include <glib.h>

#include <massifg_utils.h>


void
test_str_copy_region_substring(void) {
	gchar *str, *res, *cpy;
	str = "\
this is a string which we want to get a substring from";

	cpy = massifg_str_copy_region(str, 10, 23);
	res = "string which ";
	g_assert_cmpstr(cpy, ==, res);
}

void
test_str_copy_region_full(void) {
	gchar *str, *cpy;
	str = "this is a string which we want to get a substring from";

	cpy = massifg_str_copy_region(str, 0, strlen(str));
	g_assert_cmpstr(str, ==, cpy);
}

int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/utils/str-copy-region-substring", test_str_copy_region_substring);
	g_test_add_func("/utils/str-copy-region", test_str_copy_region_full);

	massifg_utils_configure_debug_output();
	return g_test_run();
}
