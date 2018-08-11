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


//#define LOGGER_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(logger, v)

#ifdef ZTS
#define LOGGER_G(v) TSRMG(logger_globals_id, zend_logger_globals *, v)
#else
#define LOGGER_G(v) (logger_globals.v)
#endif

#if defined(ZTS) && defined(COMPILE_DL_LOGGER)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif


#define LOGGER_CLASS_NS_NAME            "EastWood\\Log\\Logger"
#define LOGGER_CLASS_SHORT_NAME         "Logger"
#define LOGGER_CLASS_PROPERTY_NAME      "settings"

#define LOGGER_FILENAME_PREFIX          "app"
#define LOGGER_FILENAME_AFTERFIX        ".log"

#define LOGGER_LEVEL_INFO               0
#define LOGGER_LEVEL_WARNING            1
#define LOGGER_LEVEL_ERROR              2
#define LOGGER_LEVEL_DEBUG              3
#define LOGGER_LEVEL_VERBOSE            4

#define LOGGER_ROTATE_YEAR              0
#define LOGGER_ROTATE_MONTH             1
#define LOGGER_ROTATE_DAILY			        2

static zend_class_entry *logger_ce, ce;

ZEND_BEGIN_MODULE_GLOBALS(logger)
  zval async_stack;
ZEND_END_MODULE_GLOBALS(logger)

ZEND_DECLARE_MODULE_GLOBALS(logger)

PHP_METHOD(logger, info);
PHP_METHOD(logger, warning);
PHP_METHOD(logger, error);
PHP_METHOD(logger, debug);
PHP_METHOD(logger, verbose);

PHP_INI_BEGIN()
  PHP_INI_ENTRY("logger.path", "/var/log", PHP_INI_ALL, NULL)
  PHP_INI_ENTRY("logger.rotate", "", PHP_INI_ALL, NULL)
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

static int logger_rotate_intval(const char *str)
{
    if (strcmp(str, "daily") == 0) {
        return LOGGER_ROTATE_DAILY;
    }else if (strcmp(str, "month") == 0) {
        return LOGGER_ROTATE_MONTH;
    }else if (strcmp(str, "year") == 0) {
        return LOGGER_ROTATE_YEAR;
    }
    return -1;
}

static zend_string *zend_string_concat(zend_string *str1, const char *str2)
{
    zend_string *rv;
    rv = zend_strpprintf(0, "%s%s", ZSTR_VAL(str1), str2);
    zend_string_release(str1);
    return rv;
}

static zend_string *zend_string_concat_ex(zend_string *str, int count, ...)
{
    va_list ap;
    va_start(ap, count);
    while(count--) str = zend_string_concat(str, va_arg(ap, char*));
    va_end(ap);
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

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
