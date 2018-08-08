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
	| Author:                                                              |
	+----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_LOGGER_H
#define PHP_LOGGER_H

extern zend_module_entry logger_module_entry;
#define phpext_logger_ptr &logger_module_entry

#define PHP_LOGGER_VERSION "0.1.0"

#ifdef PHP_WIN32
#	define PHP_LOGGER_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LOGGER_API __attribute__ ((visibility("default")))
#else
#	define PHP_LOGGER_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


#define LOGGER_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(logger, v)

#if defined(ZTS) && defined(COMPILE_DL_LOGGER)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif

#ifndef FS_IS_W
#	define FS_IS_W	9
#endif

#ifndef FS_IS_R
#	define FS_IS_R		10
#endif

#ifndef FS_IS_X
#	define FS_IS_X		11
#endif

#ifndef FS_IS_FILE
#	define FS_IS_FILE 	12
#endif

#ifndef FS_IS_DIR
#	define FS_IS_DIR	13
#endif

#ifndef FS_IS_LINK
#	define FS_IS_LINK	14
#endif

#ifndef FS_EXISTS
#	define FS_EXISTS	15
#endif

#define LOGGER_CLASS_NS_NAME 		"EastWood\\Log\\Logger"
#define LOGGER_CLASS_SHORT_NAME 	"Logger"

#define LOGGER_LEVEL_INFO 			0
#define LOGGER_LEVEL_WARNING 		1
#define LOGGER_LEVEL_ERROR 			2
#define LOGGER_LEVEL_DEBUG 			3
#define LOGGER_LEVEL_VERBOSE 		4


extern zend_string *zend_string_append(zend_string *str1, const char *str2)
{
	int offset = ZSTR_LEN(str1);
	zend_string *rv = zend_string_extend(str1,  ZSTR_LEN(str1) + strlen(str2) + 1, 0);
	do{
		ZSTR_VAL(rv)[offset++] = *str2;
	}while(*str2++);
	ZSTR_VAL(rv)[offset++] = '\0';
	return rv;
}

extern zval* call_user_func(const char *func_name, int num, zval param[])
{
	zval rv, function, *rv_ptr;
	ZVAL_STRING(&function, func_name);
	call_user_function(EG(function_table), NULL, &function, &rv, num, param);
	rv_ptr = &rv;
	return rv_ptr;
}

static zend_class_entry *logger_ce, ce;

PHP_METHOD(logger, info);
PHP_METHOD(logger, warning);
PHP_METHOD(logger, error);
PHP_METHOD(logger, debug);
PHP_METHOD(logger, verbose);


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
