
#include <glib.h>

/* FIXME: import headers instead */
#include "../src/massifg_utils.c"
#include "../src/massifg_parser.c"

#define PARSER_TEST_INPUT_SHORT "massif-output-2snapshots.txt"
#define PARSER_TEST_INPUT_LONG "massif-output-glom.txt"

/* Utility function for finding the test input files. Especially important for distcheck
 * http://www.gnu.org/software/automake/manual/automake.html#Tests
 * Caller should free return value using g_free () */
gchar *
get_test_file(const gchar *filename) {
	gchar *srcdir = g_getenv("top_srcdir");
	gchar *path = g_build_filename(srcdir, "tests", filename, NULL);
	return path;
}


void
parser_functest_short(void) {
	MassifgOutputData *data;
	GList *list;
	MassifgSnapshot *s;

	/* Run the parser */
	gchar *path = get_test_file(PARSER_TEST_INPUT_SHORT);
	data = massifg_parse_file(path, NULL);
	g_free(path);

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

	g_assert_cmpstr(s->heap_tree_desc->str, ==, "detailed");

	/* Snapshot 1 values */
	list = g_list_nth(data->snapshots, 1);
	s = (MassifgSnapshot*) list->data;

	g_assert_cmpint(s->snapshot_no, ==, 1);

	g_assert_cmpint(s->time, ==, 46630998);
	g_assert_cmpint(s->mem_heap_B, ==, 352);
	g_assert_cmpint(s->mem_heap_extra_B, ==, 8);
	g_assert_cmpint(s->mem_stacks_B, ==, 0);

	g_assert_cmpstr(s->heap_tree_desc->str, ==, "detailed");

}

void
parser_return_null_on_nonexisting_file(void) {
	MassifgOutputData *data;

	gchar *path = get_test_file("non-existing-file.keejrsperr");
	data = massifg_parse_file(path, NULL);
	g_free(path);

	g_assert(data == NULL);
}

void
parser_return_null_on_bogus_data(void) {
	MassifgOutputData *data;

	gchar *path = get_test_file("parser.c");
	data = massifg_parse_file(path, NULL);
	g_free(path);

	g_assert(data == NULL);
}

void
parser_maxvalues(void) {
	MassifgOutputData *data;

	gchar *path = get_test_file(PARSER_TEST_INPUT_LONG);
	data = massifg_parse_file(path, NULL);
	g_free(path);

	g_assert_cmpint(data->max_time, ==, 2318524449);
	g_assert_cmpint(data->max_mem_allocation, ==, 8843592);
}

/* Detailed / Heap tree parsing tests */
void
parser_heaptree_attributes_1(void) {
	const gchar *test_str = "n13: 1411172 (heap allocation functions) malloc/new/new[], --alloc-fns, etc.";

	MassifgHeapTreeNode *node = massifg_heap_tree_node_new(test_str);
	g_assert_cmpint(node->total_mem_B, ==, 1411172);
	g_assert_cmpint(node->num_children, ==, 13);
	g_assert_cmpstr(node->label->str, ==, "(heap allocation functions) malloc/new/new[], --alloc-fns, etc.");

	g_assert_cmpint(node->parsing_depth, ==, 0);
}

void
parser_heaptree_attributes_2(void) {
	const gchar *test_str = "                         n1: 262144 0x57FDD67: import_submodule (import.c:2400)";

	MassifgHeapTreeNode *node = massifg_heap_tree_node_new(test_str);
	g_assert_cmpint(node->total_mem_B, ==, 262144);
	g_assert_cmpint(node->num_children, ==, 1);
	g_assert_cmpstr(node->label->str, ==, "0x57FDD67: import_submodule (import.c:2400)");

	g_assert_cmpint(node->parsing_depth, ==, 25);

}

void
parser_heaptree_functest(void) {
	MassifgOutputData *data;
	GList *list;
	MassifgSnapshot *s;
	MassifgHeapTreeNode *n;
	GNode *gn;
	int i = 0;

	/* Run the parser */
	gchar *path = get_test_file(PARSER_TEST_INPUT_SHORT);
	data = massifg_parse_file(path, NULL);
	g_free(path);

	/* Snapshot 0 */
	list = g_list_nth(data->snapshots, 0);
	s = (MassifgSnapshot *)list->data;

	/* This only has a tree with one node */
	n = s->heap_tree->data;
	g_assert_cmpint(n->total_mem_B, ==, 0);
	g_assert_cmpint(n->num_children, ==, 0);
	g_assert_cmpstr(n->label->str, ==, "(heap allocation functions) malloc/new/new[], --alloc-fns, etc.");

	/* Snapshot 1 */
	list = g_list_nth(data->snapshots, 1);
	s = (MassifgSnapshot *)list->data;


	/* Test an arbitrary node in this tree */
	n = s->heap_tree->children->data;
	g_assert_cmpint(n->total_mem_B, ==, 352);
	g_assert_cmpint(n->num_children, ==, 1);
	g_assert_cmpstr(n->label->str, ==, "0x5A22DDD: __fopen_internal (iofopen.c:76)");

	/* Test another node further down */
	n = s->heap_tree->children->children->children->data;
	g_assert_cmpint(n->total_mem_B, ==, 352);
	g_assert_cmpint(n->num_children, ==, 1);
	g_assert_cmpstr(n->label->str, ==, "0x54A26EE: ??? (in /lib/libselinux.so.1)");


	/* Test the last node in the tree */
	gn = s->heap_tree;
	while (gn->children) {
		gn = gn->children;
		i++;
	}

	n = gn->data;
	g_assert_cmpint(n->total_mem_B, ==, 352);
	g_assert_cmpint(n->num_children, ==, 0);
	g_assert_cmpstr(n->label->str, ==, "0x400088D: ??? (in /lib/ld-2.10.1.so)");

}

int
main (int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/parser/heaptree/functest", parser_heaptree_functest);

	g_test_add_func("/parser/functest", parser_functest_short);
	g_test_add_func("/parser/nonexisting-file", parser_return_null_on_nonexisting_file);
	g_test_add_func("/parser/bogus-data", parser_return_null_on_bogus_data);
	g_test_add_func("/parser/max-values", parser_maxvalues);

	g_test_add_func("/parser/heaptree/attributes/1", parser_heaptree_attributes_1);
	g_test_add_func("/parser/heaptree/attributes/2", parser_heaptree_attributes_2);

	massifg_utils_configure_debug_output();
	return g_test_run();
}
