/* gutenberg_post_parser extension for PHP (c) 2018 Ivan Enderlin */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_gutenberg_post_parser.h"
#include "gutenberg_post_parser.h"

/*
 * Class entry for `Gutenberg_Parser_Block` and `Gutenberg_Parser_Phrase`.
 */
zend_class_entry *gutenberg_parser_block_class_entry;
zend_class_entry *gutenberg_parser_phrase_class_entry;

/*
 * Methods for `Gutenberg_Parser_*` classes. There is no method.
 */
const zend_function_entry gutenberg_post_parser_methods[] = {
	PHP_FE_END
};

/*
 * Initialize the module.
 */
PHP_MINIT_FUNCTION(gutenberg_post_parser)
{
	zend_class_entry class_entry;

	// Declare the `Gutenberg_Parser_Block` class.
	INIT_CLASS_ENTRY(class_entry, "Gutenberg_Parser_Block", gutenberg_post_parser_methods);
	gutenberg_parser_block_class_entry = zend_register_internal_class(&class_entry TSRMLS_CC);

	// The class is final.
	gutenberg_parser_block_class_entry->ce_flags |= ZEND_ACC_FINAL;

	// Declare the `namespace` public attribute, with an empty string for the default value.
	zend_declare_property_string(gutenberg_parser_block_class_entry, "namespace", sizeof("namespace") - 1, "", ZEND_ACC_PUBLIC);

	// Declare the `name` public attribute, with an empty string for the default value.
	zend_declare_property_string(gutenberg_parser_block_class_entry, "name", sizeof("name") - 1, "", ZEND_ACC_PUBLIC);
	
	// Declare the `attributes` public attribute, with `NULL` for the default value.
	zend_declare_property_null(gutenberg_parser_block_class_entry, "attributes", sizeof("attributes") - 1, ZEND_ACC_PUBLIC);
	
	// Declare the `children` public attribute, with `NULL` for the default value.
	zend_declare_property_null(gutenberg_parser_block_class_entry, "children", sizeof("children") - 1, ZEND_ACC_PUBLIC);


	// Declare the `Gutenberg_Parser_Phrase` class.
	INIT_CLASS_ENTRY(class_entry, "Gutenberg_Parser_Phrase", gutenberg_post_parser_methods);
	gutenberg_parser_phrase_class_entry = zend_register_internal_class(&class_entry TSRMLS_CC);

	// The class is final.
	gutenberg_parser_phrase_class_entry->ce_flags |= ZEND_ACC_FINAL;

	// Declare the `content` public attribute, with an empty string for the default value.
	zend_declare_property_string(gutenberg_parser_phrase_class_entry, "content", sizeof("content") - 1, "", ZEND_ACC_PUBLIC);

	return SUCCESS;
}

PHP_RINIT_FUNCTION(gutenberg_post_parser)
{
#if defined(ZTS) && defined(COMPILE_DL_GUTENBERG_POST_PARSER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}

/*
 * Provide information about the module.
 */
PHP_MINFO_FUNCTION(gutenberg_post_parser)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "gutenberg_post_parser support", "enabled");
	php_info_print_table_end();
}

/*
 * Map Rust AST to a PHP array of objects of kind
 * `Gutenberg_Parser_Block` and `Gutenberg_Parser_Phrase`.
 */
void into_php_objects(zval *php_array, const Vector_Node* nodes)
{
	const uintptr_t number_of_nodes = nodes->length;

	if (number_of_nodes == 0) {
		return;
	}

	// Iterate over all nodes.
	for (uintptr_t nth = 0; nth < number_of_nodes; ++nth) {
		const Node node = nodes->buffer[nth];

		// Map [rust] `Node::Block` to [php] `Gutenberg_Parser_Block`.
		if (node.tag == Block) {
			const Block_Body block = node.block;
			zval php_block;

			object_init_ex(&php_block, gutenberg_parser_block_class_entry);

			// Map [rust] `Node::Block.name.0` to [php] `Gutenberg_Parser_Block->namespace`.
			add_property_string(&php_block, "namespace", block.namespace);

			// Map [rust] `Node::Block.name.1` to [php] `Gutenberg_Parser_Block->name`.
			add_property_string(&php_block, "name", block.name);

			// Default value for `Gutenberg_Parser_Block->attributes` is `NULL`.
			// Allocate a string only if some value.
			if (block.attributes.tag == Some) {
				const char *attributes = block.attributes.some._0;

				// Map [rust] `Node::Block.attributes` to [php] `Gutenberg_Parser_Block->attributes`.
				add_property_string(&php_block, "attributes", attributes);
			}

			const Vector_Node* children = (const Vector_Node*) (block.children);

			// Default value for `Gutenberg_Parser_Block->children` is `NULL`.
			// Allocate an array only if there is children.
			if (children->length > 0) {
				zval php_children_array;

				array_init(&php_children_array);
				into_php_objects(&php_children_array, children);

				// Map [rust] `Node::Block.children` to [php] `Gutenberg_Parser_Block->children`.
				add_property_zval(&php_block, "children", &php_children_array);
			}

			// Insert `Gutenberg_Parser_Block` into the collection.
			add_next_index_zval(php_array, &php_block);
		}
		// Map [rust] `Node::Phrase` to [php] `Gutenberg_Parser_Phrase`.
		else if (node.tag == Phrase) {
			const char *phrase = node.phrase._0;
			zval php_phrase;

			object_init_ex(&php_phrase, gutenberg_parser_phrase_class_entry);

			// Map [rust] `Node::Phrase(content)` to [php] `Gutenberg_Parser_Phrase->content`.
			add_property_string(&php_phrase, "content", phrase);

			// Insert `Gutenberg_Parser_Phrase` into the collection.
			add_next_index_zval(php_array, &php_phrase);
		}
	}
}

/*
 * Declare the `gutenberg_post_parse(string $gutenberg_post_as_string): bool | array;` function.
 */
PHP_FUNCTION(gutenberg_post_parse)
{
	char *input;
	size_t input_len;

	// Read the input as a string.
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &input, &input_len) == FAILURE) {
		return;
	}

	// Parse the input.
	Result parser_result = parse(input);

	// If parsing failed, then return `false`.
	if (parser_result.tag == Err) {
		RETURN_FALSE;
	}

	// Else map the Rust AST into a PHP array.
	const Vector_Node nodes = parser_result.ok._0;

	// Note: `return_value` is a “magic” variable that holds the value to be returned.
	//
	// Allocate an array.
	array_init(return_value);

	// Map the Rust AST.
	into_php_objects(return_value, &nodes);
}

/*
 * Provide arginfo.
 */
ZEND_BEGIN_ARG_INFO(arginfo_gutenberg_post_parser, 0)
	ZEND_ARG_INFO(0, gutenberg_post_as_string)
ZEND_END_ARG_INFO()

/*
 * Declare functions.
 */
static const zend_function_entry gutenberg_post_parser_functions[] = {
	PHP_FE(gutenberg_post_parse,		arginfo_gutenberg_post_parser)
	PHP_FE_END
};

/*
 * Declare the module.
 */
zend_module_entry gutenberg_post_parser_module_entry = {
	STANDARD_MODULE_HEADER,
	"gutenberg_post_parser",					/* Extension name */
	gutenberg_post_parser_functions,			/* zend_function_entry */
	PHP_MINIT(gutenberg_post_parser),			/* PHP_MINIT - Module initialization */
	NULL,										/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(gutenberg_post_parser),			/* PHP_RINIT - Request initialization */
	NULL,										/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(gutenberg_post_parser),			/* PHP_MINFO - Module info */
	PHP_GUTENBERG_POST_PARSER_VERSION,			/* Version */
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GUTENBERG_POST_PARSER
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(gutenberg_post_parser)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */