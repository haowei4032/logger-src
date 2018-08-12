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

static void logger_clean()
{
	zval *val;
	php_stream *stream;
	stream = php_stream_open_wrapper("/var/log/app-2018-08-11.log", "ab", IGNORE_PATH|IGNORE_URL_WIN, NULL);
	if (stream == NULL) return;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&LOGGER_G(async_stack)), val) {
		php_printf("%s\n", Z_STRVAL_P(val));
	} ZEND_HASH_FOREACH_END();
	php_stream_close(stream);

	zval_ptr_dtor(&LOGGER_G(async_stack));
}

static void logger_init()
{
	zval init;
	array_init_size(&init, 0);
	zend_update_static_property(logger_ce, ZEND_STRL(LOGGER_CLASS_PROPERTY_NAME), &init TSRMLS_CC);
	zval_ptr_dtor(&init);

	array_init_size(&LOGGER_G(async_stack), 0);
}

static int logger_factory(INTERNAL_FUNCTION_PARAMETERS, int level)
{
	int i, n, len;
	char format[1];
	char date_format[3] = "Ymd";

	php_stream *stream;
	zval rv, pool, filepath, rotate;
	zend_string *tag, *message, *str, *filename;

	if (ZEND_NUM_ARGS() < 2) {
		php_error_docref(NULL, E_ERROR, "expects exactly 2 parameters, %d given", ZEND_NUM_ARGS());
		return FAILURE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &tag, &message) == FAILURE) {
		return FAILURE;
	}

	ZVAL_STRING(&filepath, INI_STR("logger.path"));
	php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_DIR, &rv);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		if (!php_stream_mkdir(Z_STRVAL(filepath), 0755, PHP_STREAM_MKDIR_RECURSIVE, NULL)) {
			php_error_docref(NULL, E_ERROR, "EastWood Log directory creation failed %s", Z_STRVAL(filepath));
			return FAILURE;
		}
	}

	php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_W, &rv);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		php_error_docref(NULL, E_ERROR, "EastWood Log directory does not have write access %s", Z_STRVAL(filepath));
		return FAILURE;
	}

	array_init(&pool);
	add_next_index_string(&pool, LOGGER_FILENAME_PREFIX);

	ZVAL_STRING(&rotate, INI_STR("logger.rotate"));

	for(i = 0; i <= logger_rotate_intval(Z_STRVAL(rotate)); i++) {
		format[0] = date_format[i];
		str = php_format_date(format, 1, time(NULL), 1);
		add_next_index_string(&pool, ZSTR_VAL(str));
		zend_string_release(str);
	}

	str = zend_string_init("-", 1, 0);
	php_implode(str, &pool, &rv);
	zend_string_release(str);

	str = zend_string_init(Z_STRVAL(filepath), Z_STRLEN(filepath), 0);
	filename = zend_string_concat_ex(str, 3, "/", Z_STRVAL(rv), LOGGER_FILENAME_AFTERFIX);

	zval_ptr_dtor(&rv);
	php_stat(ZSTR_VAL(filename), ZSTR_LEN(filename), FS_IS_FILE, &rv);
	if (Z_TYPE_P(&rv) == IS_TRUE) {
		//zval_ptr_dtor(&rv);
		php_stat(ZSTR_VAL(filename), ZSTR_LEN(filename), FS_IS_W, &rv);
		if (Z_TYPE_P(&rv) == IS_FALSE) {
			php_error_docref(NULL, E_ERROR, "EastWood Log file has no write permission %s", ZSTR_VAL(filename));
			return FAILURE;
		}
	}

	add_next_index_string(&LOGGER_G(async_stack), ZSTR_VAL(message));

	/*stream = php_stream_open_wrapper(ZSTR_VAL(filename), "ab", IGNORE_PATH|IGNORE_URL_WIN, NULL);
	if (stream == NULL) return FAILURE;

	len = ZSTR_LEN(message);
	message = zend_string_concat(message, "\n");
	php_stream_write(stream, ZSTR_VAL(message), ZSTR_LEN(message));
	php_stream_close(stream);*/

	zend_string_release(str);
	zend_string_release(message);
	zend_string_release(filename);
	
	zval_ptr_dtor(&rv);
	zval_ptr_dtor(&pool);
	zval_ptr_dtor(&rotate);
	zval_ptr_dtor(&filepath);

	return len;
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
	logger_clean();
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
