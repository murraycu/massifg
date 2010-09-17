
#ifndef MASSIFG_PARSER_PRIVATE_H__
#define MASSIFG_PARSER_PRIVATE_H__

/* Only for testing */

MassifgHeapTreeNode *massifg_heap_tree_node_new(const gchar *line);

void massifg_heap_tree_node_free(MassifgHeapTreeNode *node);

#endif /* MASSIFG_PARSER_PRIVATE_H__ */
