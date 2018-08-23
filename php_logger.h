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
#include "main/SAPI.h"
#include <ext/date/php_date.h>
#include <ext/standard/php_string.h>
#include <ext/standard/php_filestat.h>

#ifndef PHP_LOGGER_H
#define PHP_LOGGER_H

extern zend_module_entry logger_module_entry;
#define phpext_logger_ptr &logger_module_entry

#define PHP_LOGGER_VERSION "1.0.0"

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

#define LOGGER_LEVEL_INFO               0
#define LOGGER_LEVEL_WARNING            1
#define LOGGER_LEVEL_ERROR              2
#define LOGGER_LEVEL_DEBUG              3
#define LOGGER_LEVEL_VERBOSE            4

#define LOGGER_ROTATE_YEAR              0
#define LOGGER_ROTATE_MONTH             1
#define LOGGER_ROTATE_DAILY             2

static zend_class_entry *logger_ce, ce;

ZEND_BEGIN_MODULE_GLOBALS(logger)
    zval logging_stack;
    zval logging_filename;
ZEND_END_MODULE_GLOBALS(logger)

ZEND_DECLARE_MODULE_GLOBALS(logger)

PHP_FUNCTION(logger_version);

PHP_METHOD(logger, info);
PHP_METHOD(logger, warning);
PHP_METHOD(logger, error);
PHP_METHOD(logger, debug);
PHP_METHOD(logger, verbose);

PHP_INI_BEGIN()
    PHP_INI_ENTRY("logger.path", "/var/log", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("logger.rotate", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("logger.format", "{date_rfc}||{timestamp}||{level}||{tag}||{message}", PHP_INI_ALL, NULL)
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
    PHP_FE(logger_version, NULL)
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

char* string_replace(char *source, char *search, char *replace)
{
    int pos;
    int offset;
    int source_len = strlen(source);
    int search_len = strlen(search);
    int replace_len = strlen(replace);
    char *buf = NULL;
    char *needle;

    while((needle = strstr(source, search))) {
        offset = 0;
        //printf("in buf: %s\n", source);
        pos = needle - source;
        //printf("needle: %s pos: %d\n", needle, pos);
        buf = (char*)malloc(source_len + replace_len);
        memcpy(buf, source, pos);
        //printf("cpy buf: %s\n", buf);
        offset += pos;
        memcpy(buf + offset, replace, replace_len);
        //printf("rpy buf: %s\n", buf);
        offset += replace_len;
        memcpy(buf + offset, source + pos + search_len, source_len - pos - search_len);
        offset += source_len - pos - search_len;
        source = strdup(buf);
        source_len = strlen(source);
        buf[offset] = '\0';
        //printf("output buf: %s\n", buf);
    }
    if (buf == NULL) buf = strdup(source);
    
    return buf;
}


static char* string_sprintf(const char *format, ...)
{
    char *s;
    va_list args;
    va_start(args, format);
    vsprintf(s, format, args);
    va_end(args);
    return s;
}

static zend_string* string_trim(const char *subject)
{
    zend_string *tmp = zend_string_init(subject, strlen(subject), 0);
    zend_string *result = php_trim(tmp, NULL, 0, 3);
    zend_string_free(tmp);
    return result;
}

static double timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec / 1000000;
}

static zend_string *get_current_user()
{
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    return zend_string_init(pwd->pw_name, strlen(pwd->pw_name), 0);
}

static zend_string *_gethostname()
{
    char buf[255];
    gethostname(buf, sizeof(buf));
    return zend_string_init(buf, strlen(buf), 0);
}

static zval* get_server_var(const char* key)
{
    zval val, *ht, *value;
    ZVAL_STRING(&val, "-");
    ht = zend_hash_str_find(&EG(symbol_table), ZEND_STRL("_SERVER"));
    value = zend_hash_str_find(Z_ARRVAL_P(ht), key, strlen(key));
    if (value == NULL) value = &val;
    if (Z_TYPE_P(value) != IS_STRING) convert_to_string(value);
    zval_ptr_dtor(&val);
    return value;
}

static zend_string* get_post_body()
{
    zend_long maxlen = (ssize_t) PHP_STREAM_COPY_ALL;
    php_stream *stream = php_stream_open_wrapper("php://input", "rb", REPORT_ERRORS, NULL);
    zend_string *body = php_stream_copy_to_mem(stream, maxlen, 0);
    php_stream_close(stream);
    return body == NULL ? ZSTR_EMPTY_ALLOC() : body;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
