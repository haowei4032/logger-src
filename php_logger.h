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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include <ext/date/php_date.h>
#include <ext/standard/info.h>
#include <ext/standard/php_filestat.h>

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
#	define FS_IS_W		9
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

#define LOGGER_FILENAME_PREFIX      "app"
#define LOGGER_FILENAME_AFTERFIX    ".log"

#define LOGGER_LEVEL_INFO 			0
#define LOGGER_LEVEL_WARNING 		1
#define LOGGER_LEVEL_ERROR 			2
#define LOGGER_LEVEL_DEBUG 			3
#define LOGGER_LEVEL_VERBOSE 		4

#define LOGGER_ROTATE_DAILY			0
#define	LOGGER_ROTATE_MONTH			1
#define	LOGGER_ROTATE_YEAR			2

static int logger_rotate_intval(const char *str)
{
    if (strcmp(str, "daily") == 0) {
        return 0;
    }else if (strcmp(str, "month") == 0) {
        return 1;
    }else if (strcmp(str, "year") == 0) {
        return 2;
    }
    return -1;
}

static zend_string *zend_string_concat(zend_string *str1, const char *str2)
{
    zval v;
    ZVAL_STRING(&v, str2);
    zend_string *str;
    str = zend_string_init(strcat(ZSTR_VAL(str1), str2), ZSTR_LEN(str1) + Z_STRLEN(v), 0);
    zend_string_release(str1);
    zval_ptr_dtor(&v);
    return str;
}

static zval* call_user_func(const char *func_name, int num, zval param[])
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
