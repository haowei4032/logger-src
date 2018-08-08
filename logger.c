#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_logger.h"

PHP_INI_BEGIN()
	PHP_INI_ENTRY("logger.path", "/var/log", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.rotate", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.format", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("logger.application", "", PHP_INI_ALL, NULL)
PHP_INI_END()

const zend_function_entry logger_methods[] = {
	//PHP_ME(logger, set, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	//PHP_ME(logger, step, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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

	zval rv, param[0];
	ZVAL_STRING(&param[0], INI_STR("logger.path"));
	php_stat(Z_STRVAL(param[0]), Z_STRLEN(param[0]), FS_IS_DIR, &rv);
	//php_var_dump(&rv, 1);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		if (!php_stream_mkdir(Z_STRVAL(param[0]), 0755, PHP_STREAM_MKDIR_RECURSIVE, NULL)) {
			php_error_docref(NULL, E_ERROR, "EastWood Log directory creation failed %s", Z_STRVAL(param[0]));
			return FAILURE;
		}
	}

	php_stat(Z_STRVAL(param[0]), Z_STRLEN(param[0]), FS_IS_W, &rv);
	//php_var_dump(&rv, 1);
	if (Z_TYPE_P(&rv) == IS_FALSE) {
		php_error_docref(NULL, E_ERROR, "EastWood Log directory is reject write permission %s", Z_STRVAL(param[0]));
		return FAILURE;
	}

	zend_string *path = zend_string_init(Z_STRVAL(param[0]), Z_STRLEN(param[0]), 0);
	path = zend_string_append(path, "/abc.log");
	stream = php_stream_open_wrapper(ZSTR_VAL(path), "ab", IGNORE_PATH|IGNORE_URL_WIN, NULL);
	if (stream == NULL) return FAILURE;
	message = zend_string_append(message, "\n");
	php_stream_write(stream, ZSTR_VAL(message), ZSTR_LEN(message));
	php_stream_close(stream);

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
	//php_info_print_table_header(2, "logger support", "enabled");
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

