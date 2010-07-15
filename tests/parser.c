
#include <glib.h>

/* FIXME: import headers instead */
#include "../src/massifg_utils.c"
#include "../src/massifg_parser.c"

#define PARSER_TEST_INPUT_SHORT "tests/massif-output-2snapshots.txt"


void
parser_functest_short(void) {
	MassifgOutputData *data;
	GList *list;
	MassifgSnapshot *s;

	/* Silence DEBUG messages */
	g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, massifg_utils_log_ignore, NULL);

	/* Run the parser */
	data = massifg_parse_file(PARSER_TEST_INPUT_SHORT);

	/* Header values */
	g_assert_cmpstr(data->desc->str, ==, "--detailed-freq=1");
	g_assert_cmpstr(data->cmd->str, ==, "glom");
	g_assert_cmpstr(data->time_unit->str, ==, "i");

	/* Number of snapshots */
	g_assert_cmpint(g_list_length(data->snapshots), ==, 2);

	/* Snapshot 0 values */
	list = g_list_nth(data->snapshots, 0);
	s = (MassifgSnapshot*) list->data;

	g_assert_cmpint(s->snapshot_no, ==, 0);

	g_assert_cmpint(s->time, ==, 0);
	g_assert_cmpint(s->mem_heap_B, ==, 0);
	g_assert_cmpint(s->mem_heap_extra_B, ==, 0);
	g_assert_cmpint(s->mem_stacks_B, ==, 0);

	/* Snapshot 1 values */
	list = g_list_nth(data->snapshots, 1);
	s = (MassifgSnapshot*) list->data;

	g_assert_cmpint(s->snapshot_no, ==, 1);

	g_assert_cmpint(s->time, ==, 46630998);
	g_assert_cmpint(s->mem_heap_B, ==, 352);
	g_assert_cmpint(s->mem_heap_extra_B, ==, 8);
	g_assert_cmpint(s->mem_stacks_B, ==, 0);

}


int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/parser/functest", parser_functest_short);

	return g_test_run();
}
