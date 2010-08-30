
#include <glib.h>

#include <massifg_graph.h>
#include <massifg_utils.h>

void
test_short_function_label(void) {
	gchar *str;
	str = get_short_function_label("function (type, tupe, type) (in blabla something.so)");

	str = get_short_function_label("std::string::_Rep::_S_create(unsigned int, unsigned int, std::allocator<char> const&) (in /usr/lib/libstdc++.so.6.0.13)");
	g_assert_cmpstr(str, ==, "std::string::_Rep::_S_create (in /usr/lib/libstdc++.so.6.0.13)");

}

int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/graph/internal/short-function-label", test_short_function_label);

	massifg_utils_configure_debug_output();
	return g_test_run();
}
