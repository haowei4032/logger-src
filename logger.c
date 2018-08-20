/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: eastwood<boss@haowei.me>                                     |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_logger.h"

static void php_logger_init_globals(zend_logger_globals *logger_globals TSRMLS_DC)
{}

static void logger_environment_init()
{
	int i, n, len;
	char format[1];
	char date_format[3] = "Ymd";

	zval rv, list, rotate, filepath;
	zend_string *str, *filename;

	ZVAL_STRING(&filepath, INI_STR("logger.path"));
	php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_DIR, &rv);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		if (!php_stream_mkdir(Z_STRVAL(filepath), 0755, PHP_STREAM_MKDIR_RECURSIVE, NULL)) {
			php_error_docref(NULL, E_ERROR, "EastWood Log directory creation failed %s", Z_STRVAL(filepath));
			return ;
		}
	}

	php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_W, &rv);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		php_error_docref(NULL, E_ERROR, "EastWood Log directory does not have write access %s", Z_STRVAL(filepath));
		return ;
	}

	array_init(&list);
	add_next_index_string(&list, LOGGER_FILENAME_PREFIX);

	ZVAL_STRING(&rotate, INI_STR("logger.rotate"));

	for(i = 0; i <= logger_rotate_intval(Z_STRVAL(rotate)); i++) {
		format[0] = date_format[i];
		str = php_format_date(format, 1, time(NULL), 1);
		add_next_index_string(&list, ZSTR_VAL(str));
		zend_string_release(str);
	}

	str = zend_string_init("-", 1, 0);
	php_implode(str, &list, &rv);
	
	zval_ptr_dtor(&rv);
	zend_string_release(str);

	str = zend_string_init(Z_STRVAL(filepath), Z_STRLEN(filepath), 0);
	filename = zend_string_concat_ex(str, 3, "/", Z_STRVAL(rv), LOGGER_FILENAME_AFTERFIX);
	zend_string_release(str);

	php_stat(ZSTR_VAL(filename), ZSTR_LEN(filename), FS_IS_FILE, &rv);
	if (Z_TYPE_P(&rv) == IS_TRUE) {
		//zval_ptr_dtor(&rv);
		php_stat(ZSTR_VAL(filename), ZSTR_LEN(filename), FS_IS_W, &rv);
		if (Z_TYPE_P(&rv) == IS_FALSE) {
			php_error_docref(NULL, E_ERROR, "EastWood Log file has no write permission %s", ZSTR_VAL(filename));
			return ;
		}
	}

	//ZVAL_STR(&LOGGER_G(logging_filename), filename);
	LOGGER_G(logging_filename) = ZSTR_VAL(filename);
	php_printf("filename.first: %s\n", LOGGER_G(logging_filename));

	zval_ptr_dtor(&rv);
	zval_ptr_dtor(&list);
	zval_ptr_dtor(&rotate);
	zval_ptr_dtor(&filepath);
	zend_string_release(filename);
}

static void logger_init()
{
	zval init;
	array_init_size(&init, 0);
	zend_update_static_property(logger_ce, ZEND_STRL(LOGGER_CLASS_PROPERTY_NAME), &init TSRMLS_CC);
	zval_ptr_dtor(&init);

	array_init_size(&LOGGER_G(logging_stack), 0);

}

static void logger_cleaner()
{
	php_printf("filename.last: %s\n", LOGGER_G(logging_filename));

	zval *val;
	//php_stream *stream = php_stream_open_wrapper(Z_STRVAL(LOGGER_G(logging_filename)), "ab", IGNORE_PATH|IGNORE_URL_WIN, NULL);
	//if (stream == NULL) return;
	//php_printf("filename: %s\n", Z_STRVAL_P(&LOGGER_G(logging_filename)));
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&LOGGER_G(logging_stack)), val) {
		php_printf("yield: %s\n", Z_STRVAL_P(val));
		//php_stream_write(stream, Z_STRVAL_P(val), Z_STRLEN_P(val));
		//zval_ptr_dtor(val);
	} ZEND_HASH_FOREACH_END();
	//php_stream_close(stream);

	//zval_ptr_dtor(&LOGGER_G(logging_filename));
	zval_ptr_dtor(&LOGGER_G(logging_stack));
}

static void logger_format(zval *result, char *search, size_t search_len, char *replace, size_t replace_len)
{
	zend_string *rv = php_str_to_str(
		Z_STRVAL_P(result),
		Z_STRLEN_P(result),
		search,
		search_len,
		replace,
		replace_len
	);
	zval_ptr_dtor(result);
	ZVAL_STR(result, rv);
	zend_string_free(rv);
}

static int logger_factory(INTERNAL_FUNCTION_PARAMETERS, int level)
{
	zval rv, level_list;
	zend_string *tmp, *tag, *message, *output;

	array_init(&level_list);
	add_next_index_string(&level_list, LOGGER_LEVEL_STR_INFO);
	add_next_index_string(&level_list, LOGGER_LEVEL_STR_WARNING);
	add_next_index_string(&level_list, LOGGER_LEVEL_STR_ERROR);
	add_next_index_string(&level_list, LOGGER_LEVEL_STR_DEBUG);
	add_next_index_string(&level_list, LOGGER_LEVEL_STR_VERBOSE);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &tag, &message) == FAILURE) {
		return FAILURE;
	}

	zval_ptr_dtor(&rv);

	logger_environment_init();

	php_printf("filename.second: %s\n", LOGGER_G(logging_filename));

	if (Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY) {
		php_printf("_SERVER is exists\n");
	}



	ZVAL_STRING(&rv, INI_STR("logger.format"));
	//php_printf("format: %s\n", Z_STRVAL(rv));



	string_trim(INI_STR("logger.application"), &tmp);
	output = zend_strpprintf(0, "%s", ZSTR_LEN(tmp) == 0 ? "-" : ZSTR_VAL(tmp));
	logger_format(&rv, "{app}", sizeof("{app}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	logger_format(&rv, "{application}", sizeof("{application}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(tmp);
	zend_string_release(output);

	output = php_format_date("r", 1, time(NULL), 1);
	logger_format(&rv, "{date_rfc}", sizeof("{date_rfc}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	php_printf("filename.third: %s\n", LOGGER_G(logging_filename));

	output = get_host_name();
	logger_format(&rv, "{hostname}", sizeof("{hostname}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	output = zend_strpprintf(0, "%s", get_current_user());
	logger_format(&rv, "{current_user}", sizeof("{current_user}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	output = zend_strpprintf(0, "%.3f", microtime());
	logger_format(&rv, "{timestamp}", sizeof("{timestamp}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	zval *val = zend_hash_index_find(Z_ARRVAL(level_list), level);
	output = zend_strpprintf(0, "%s", Z_STRVAL_P(val));
	logger_format(&rv, "{level}", sizeof("{level}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zval_ptr_dtor(val);
	zend_string_release(output);

	output = zend_strpprintf(0, "%d", getgid());
	logger_format(&rv, "{gid}", sizeof("{gid}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	output = zend_strpprintf(0, "%d", getpid());
	logger_format(&rv, "{pid}", sizeof("{pid}") - 1, ZSTR_VAL(output), ZSTR_LEN(output));
	zend_string_release(output);

	logger_format(&rv, "{tag}", sizeof("{tag}") - 1, ZSTR_VAL(tag), ZSTR_LEN(tag));
	logger_format(&rv, "{message}", sizeof("{message}") - 1, ZSTR_VAL(message), ZSTR_LEN(message));

	


	add_next_index_string(&LOGGER_G(logging_stack), Z_STRVAL(rv));
	zval_ptr_dtor(&rv);
	zval_ptr_dtor(&level_list);

	php_printf("filename.fouth: %s\n", LOGGER_G(logging_filename));

	return SUCCESS;
}


PHP_FUNCTION(logger_version)
{
	RETURN_STRING(PHP_LOGGER_VERSION);
}

PHP_METHOD(logger, info)
{
	RETURN_LONG(logger_factory(INTERNAL_FUNCTION_PARAM_PASSTHRU, LOGGER_LEVEL_INFO));
}

PHP_METHOD(logger, warning)
{
	RETURN_LONG(logger_factory(INTERNAL_FUNCTION_PARAM_PASSTHRU, LOGGER_LEVEL_WARNING));
}

PHP_METHOD(logger, error)
{
	RETURN_LONG(logger_factory(INTERNAL_FUNCTION_PARAM_PASSTHRU, LOGGER_LEVEL_ERROR));
}

PHP_METHOD(logger, debug)
{
	RETURN_LONG(logger_factory(INTERNAL_FUNCTION_PARAM_PASSTHRU, LOGGER_LEVEL_DEBUG));
}

PHP_METHOD(logger, verbose)
{
	RETURN_LONG(logger_factory(INTERNAL_FUNCTION_PARAM_PASSTHRU, LOGGER_LEVEL_VERBOSE));
}

PHP_MINIT_FUNCTION(logger)
{
	ZEND_INIT_MODULE_GLOBALS(logger, php_logger_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	INIT_CLASS_ENTRY(ce, LOGGER_CLASS_NS_NAME, logger_methods);
	logger_ce = zend_register_internal_class(&ce TSRMLS_CC);
	zend_declare_property_null(logger_ce, ZEND_STRL(LOGGER_CLASS_PROPERTY_NAME), ZEND_ACC_STATIC|ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_register_class_alias(LOGGER_CLASS_SHORT_NAME, logger_ce);
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(logger)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(logger)
{
#if defined(COMPILE_DL_LOGGER) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	logger_init();
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(logger)
{
	logger_cleaner();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(logger)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "logger support", "enabled");
	php_info_print_table_row(2, "logger version", PHP_LOGGER_VERSION);
	php_info_print_table_row(2, "logger author", "eastwood<boss@haowei.me>");
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}

zend_module_entry logger_module_entry = {
	STANDARD_MODULE_HEADER,
	"logger",
	logger_functions,
	PHP_MINIT(logger),
	PHP_MSHUTDOWN(logger),
	PHP_RINIT(logger),	
	PHP_RSHUTDOWN(logger),
	PHP_MINFO(logger),
	PHP_LOGGER_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_LOGGER
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(logger)
#endif