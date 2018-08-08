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
  | Author: eastwood<boss@haowei.me>                                                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_logger.h"

PHP_INI_BEGIN()
	PHP_INI_ENTRY("logger.path", "/var/log", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.rotate", "daily", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.format", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.application", "", PHP_INI_ALL, NULL)
PHP_INI_END()

const zend_function_entry logger_methods[] = {
	PHP_ME(logger, info, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(logger, warning, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(logger, error, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(logger, debug, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(logger, verbose, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};

const zend_function_entry logger_functions[] = {
    PHP_FE_END
};

static int logger_factory(INTERNAL_FUNCTION_PARAMETERS, int level)
{
	php_stream *stream;
	zend_string *tag;
	zend_string *message;

	if (ZEND_NUM_ARGS() < 2) {
		php_error_docref(NULL, E_ERROR, "expects exactly 2 parameters, %d given", ZEND_NUM_ARGS());
		return FAILURE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &tag, &message) == FAILURE) {
		return FAILURE;
	}

	zval rv, pool, filepath, rotate;
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
		php_error_docref(NULL, E_ERROR, "EastWood Log directory is reject write permission %s", Z_STRVAL(filepath));
		return FAILURE;
	}

	array_init(&pool);
	add_next_index_string(&pool, LOGGER_FILENAME_PREFIX);

	ZVAL_STRING(&rotate, INI_STR("logger.rotate"));

	//php_printf("rotate: %d\n", logger_rotate_intval(Z_STRVAL(rotate)));
	zend_string *str, *filename;
	char dt[3] = "Ymd";
	char f[1];
	int i, n;
	switch(logger_rotate_intval(Z_STRVAL(rotate))) {
		case LOGGER_ROTATE_DAILY:
			n = 3;
			for(i = 0; i < n; i++) {
				f[0] = dt[i];
				str = php_format_date(f, 1, time(NULL), 1);
				add_next_index_string(&pool, ZSTR_VAL(str));
				zend_string_release(str);
			}
			break;
		case LOGGER_ROTATE_MONTH:
			n = 2;
			for(i = 0; i < n; i++) {
				f[0] = dt[i];
				str = php_format_date(f, 1, time(NULL), 1);
				add_next_index_string(&pool, ZSTR_VAL(str));
				zend_string_release(str);
			}
			break;
		case LOGGER_ROTATE_YEAR:
			n = 1;
			for(i = 0; i < n; i++) {
				f[0] = dt[i];
				str = php_format_date(f, 1, time(NULL), 1);
				add_next_index_string(&pool, ZSTR_VAL(str));
				zend_string_release(str);
			}
			break;
	}

	str = zend_string_init("-", 1, 0);
	php_implode(str, &pool, &rv);

	filename = zend_string_init(Z_STRVAL(filepath), Z_STRLEN(filepath), 0);
	filename = zend_string_concat(filename, "/");
	filename = zend_string_concat(filename, Z_STRVAL(rv));
	filename = zend_string_concat(filename, LOGGER_FILENAME_AFTERFIX);
	//php_printf("filename: %s\n", ZSTR_VAL(filename));

	stream = php_stream_open_wrapper(ZSTR_VAL(filename), "ab", IGNORE_PATH|IGNORE_URL_WIN, NULL);
	if (stream == NULL) return FAILURE;
	message = zend_string_concat(message, "\n");
	php_stream_write(stream, ZSTR_VAL(message), ZSTR_LEN(message));
	php_stream_close(stream);

	zval_ptr_dtor(&rv);
	zval_ptr_dtor(&pool);
	zval_ptr_dtor(&rotate);
	zval_ptr_dtor(&filepath);
	zend_string_release(str);
	zend_string_release(message);
	zend_string_release(filename);

	return SUCCESS;
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
	REGISTER_INI_ENTRIES();

	INIT_CLASS_ENTRY(ce, LOGGER_CLASS_NS_NAME, logger_methods);
	logger_ce = zend_register_internal_class(&ce TSRMLS_CC);
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
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(logger)
{
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

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
