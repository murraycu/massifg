
#include <glib.h>
#include <glib/gstdio.h>

#include <massifg_graph.h>
#include <massifg_graph_private.h>
#include <massifg_utils.h>

#include "common.h"

void
test_short_function_label(void) {
	gchar *str;
	str = get_short_function_label("function (type, tupe, type) (in blabla something.so)");

	str = get_short_function_label("std::string::_Rep::_S_create(unsigned int, unsigned int, std::allocator<char> const&) (in /usr/lib/libstdc++.so.6.0.13)");
	g_assert_cmpstr(str, ==, "std::string::_Rep::_S_create (in /usr/lib/libstdc++.so.6.0.13)");

}

void
graph_save_png(void) {
	MassifgOutputData *data;
	MassifgGraph *graph;
	gchar *output_path = "tests/graph-save.png";
	gchar *path = get_test_file(TEST_INPUT_LONG);

	data = massifg_parse_file(path, NULL);
	g_free(path);
	graph = massifg_graph_new();
	massifg_graph_set_data(graph, data);

	g_assert(!g_file_test(output_path, G_FILE_TEST_IS_REGULAR));

	massifg_graph_render_to_png(graph, output_path, 1000, 1000);
	g_assert(g_file_test(output_path, G_FILE_TEST_IS_REGULAR));

	g_unlink(output_path);
}

int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/graph/internal/short-function-label", test_short_function_label);

	g_test_add_func("/graph/render-to-png", graph_save_png);

	massifg_utils_configure_debug_output();
	massifg_graph_init();
	return g_test_run();
}
