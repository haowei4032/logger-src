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

static zval* logger_get_variable(INTERNAL_FUNCTION_PARAMETERS)
{
    zval *v;
    array_init_size(v, 0);
    return v;
}

static void logger_begin()
{
    zend_string *server = zend_string_init(ZEND_STRL("_SERVER"), 0);
    zend_is_auto_global(server);
    zend_string_free(server);

    zval init;
    array_init_size(&init, 0);
    zend_update_static_property(logger_ce, ZEND_STRL("settings"), &init TSRMLS_CC);
    zval_ptr_dtor(&init);

    array_init_size(&LOGGER_G(logging_stack), 0);
}

static void logger_end()
{
    zval *val;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&LOGGER_G(logging_stack)), val) {
        php_printf("yield: %s\n", Z_STRVAL_P(val));
    } ZEND_HASH_FOREACH_END();

    php_printf("filename.last: %s\n", Z_STRVAL(LOGGER_G(logging_filename)));

    zval_ptr_dtor(val);
    zval_ptr_dtor(&LOGGER_G(logging_filename));
    zval_ptr_dtor(&LOGGER_G(logging_stack));
}

static void logger_format_line(zval *line, char *search, char *replace)
{
    char *value = string_replace(Z_STRVAL_P(line), search, replace);
    zval_ptr_dtor(line);
    ZVAL_STRING(line, value);
    free(value);
}

static void logger_environment_init()
{
    int i, n, len;
    char format[1];
    char date_format[3] = "Ymd";

    zval rv, list, rotate, filepath, filename;
    zend_string *str;

    ZVAL_STRING(&filepath, INI_STR("logger.path"));
    php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_DIR, &rv);
    if (Z_TYPE_P(&rv) == IS_FALSE) {
        if (!php_stream_mkdir(Z_STRVAL(filepath), 0755, PHP_STREAM_MKDIR_RECURSIVE, NULL)) {
            php_error_docref(NULL, E_ERROR, "EastWood Log directory creation failed %s", Z_STRVAL(filepath));
            return;
        }
    }

    php_stat(Z_STRVAL(filepath), Z_STRLEN(filepath), FS_IS_W, &rv);
    if (Z_TYPE_P(&rv) == IS_FALSE) {
        php_error_docref(NULL, E_ERROR, "EastWood Log directory does not have write access %s", Z_STRVAL(filepath));
        return;
    }

    array_init(&list);
    add_next_index_string(&list, "app");

    ZVAL_STRING(&rotate, INI_STR("logger.rotate"));
    for(i = 0; i <= logger_rotate_intval(Z_STRVAL(rotate)); i++) {
        format[0] = date_format[i];
        str = php_format_date(format, 1, time(NULL), 1);
        add_next_index_string(&list, ZSTR_VAL(str));
        zend_string_release(str);
    }

    str = zend_string_init("-", 1, 0);
    php_implode(str, &list, &rv);
    zend_string_free(str);

    ZVAL_STRING(&filename, string_sprintf("%s/%s.log", Z_STRVAL(filepath), Z_STRVAL(rv)));
    zval_ptr_dtor(&rv);

    php_stat(Z_STRVAL(filename), Z_STRLEN(filename), FS_IS_FILE, &rv);
    if (Z_TYPE_P(&rv) == IS_TRUE) {
        php_stat(Z_STRVAL(filename), Z_STRLEN(filename), FS_IS_W, &rv);
        if (Z_TYPE_P(&rv) == IS_FALSE) {
            php_error_docref(NULL, E_ERROR, "EastWood Log file has no write permission %s", Z_STRVAL(filename));
            return;
        }
    }

    ZVAL_STRING(&LOGGER_G(logging_filename), Z_STRVAL(filename));

    zval_ptr_dtor(&rv);
    zval_ptr_dtor(&list);
    zval_ptr_dtor(&rotate);
    zval_ptr_dtor(&filepath);
    zval_ptr_dtor(&filename);
}

static int logger_factory(INTERNAL_FUNCTION_PARAMETERS, int level)
{
    zval rv, *var, log_level;
    zend_string *tag, *message, *output;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &tag, &message) == FAILURE) {
        return FAILURE;
    }

    logger_environment_init();

    array_init(&log_level);
    add_next_index_string(&log_level, "info");
    add_next_index_string(&log_level, "warning");
    add_next_index_string(&log_level, "error");
    add_next_index_string(&log_level, "debug");
    add_next_index_string(&log_level, "verbose");

    ZVAL_STRING(&rv, INI_STR("logger.format"));
    php_printf("format: %s\n", Z_STRVAL(rv));

    var = get_server_var("HTTP_HOST");
    logger_format_line(&rv, "{http_host}", Z_STRVAL_P(var));
    logger_format_line(&rv, "{request_host}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("REQUEST_URI");
    logger_format_line(&rv, "{uri}", Z_STRVAL_P(var));
    logger_format_line(&rv, "{request_uri}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("REQUEST_METHOD");
    logger_format_line(&rv, "{verb}", Z_STRVAL_P(var));
    logger_format_line(&rv, "{request_method}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("REMOTE_ADDR");
    logger_format_line(&rv, "{request_addr}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("REMOTE_PORT");
    logger_format_line(&rv, "{request_addr}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("SERVER_ADDR");
    logger_format_line(&rv, "{server_addr}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("SERVER_PORT");
    logger_format_line(&rv, "{server_port}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("SERVER_PROTOCOL");
    logger_format_line(&rv, "{server_protocol}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    var = get_server_var("HTTPS");
    logger_format_line(&rv, "{request_scheme}", strcmp(Z_STRVAL_P(var), "-") == 0 ? "https" : "http" );
    //zval_ptr_dtor(var);

    //var = get_server_var("HTTP_X_FORWARDED_PROTO");
    //logger_format_line(&rv, "{request_scheme}", strcmp(Z_STRVAL_P(var), "-") == 0 ? "http" : Z_STRVAL_P(var) );
    //zval_ptr_dtor(var);

    var = get_server_var("HTTP_USER_AGENT");
    logger_format_line(&rv, "{user_agent}", Z_STRVAL_P(var));
    logger_format_line(&rv, "{request_user_agent}", Z_STRVAL_P(var));
    //zval_ptr_dtor(var);

    output = string_trim(INI_STR("logger.application"));
    logger_format_line(&rv, "{app}", ZSTR_LEN(output) == 0 ? "-" : ZSTR_VAL(output));
    logger_format_line(&rv, "{application}", ZSTR_LEN(output) == 0 ? "-" : ZSTR_VAL(output));
    zend_string_release(output);

    output = get_post_body();
    php_printf("post_body: %s\n", ZSTR_LEN(output) == 0 ? "-" : ZSTR_VAL(output));
    logger_format_line(&rv, "{request_body}", ZSTR_LEN(output) == 0 ? "-" : ZSTR_VAL(output));
    zend_string_release(output);

    output = _gethostname();
    logger_format_line(&rv, "{hostname}", ZSTR_VAL(output));
    zend_string_release(output);

    output = zend_strpprintf(0, "%d", getgid());
    logger_format_line(&rv, "{gid}", ZSTR_VAL(output));
    zend_string_release(output);

    output = zend_strpprintf(0, "%d", getpid());
    logger_format_line(&rv, "{pid}", ZSTR_VAL(output));
    zend_string_release(output);

    output = php_format_date("r", 1, time(NULL), 1);
    logger_format_line(&rv, "{date_rfc}", ZSTR_VAL(output));
    zend_string_release(output);

    output = zend_strpprintf(0, "%.3f", timestamp());
    logger_format_line(&rv, "{timestamp}", ZSTR_VAL(output));
    zend_string_release(output);

    output = zend_hash_index_find_ptr(Z_ARRVAL(log_level), level);
    logger_format_line(&rv, "{level}", ZSTR_VAL(output));
    zend_string_release(output);

    output = get_current_user();
    logger_format_line(&rv, "{current_user}", ZSTR_VAL(output));
    zend_string_release(output);

    logger_format_line(&rv, "{tag}", ZSTR_VAL(tag));
    logger_format_line(&rv, "{message}", ZSTR_VAL(message));

    add_next_index_string(&LOGGER_G(logging_stack), Z_STRVAL(rv));
    zval_ptr_dtor(&rv);
    zval_ptr_dtor(&log_level);

    return SUCCESS;
}


PHP_FUNCTION(logger_version)
{
    RETURN_STRING(PHP_LOGGER_VERSION);
}

PHP_METHOD(logger, set)
{
    RETURN_NULL();
}

PHP_METHOD(logger, getVariable)
{
    //logger_get_variable(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    RETURN_NULL();
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
    INIT_CLASS_ENTRY(ce, "EastWood\\Log\\Logger", logger_methods);
    logger_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_declare_property_null(logger_ce, ZEND_STRL("settings"), ZEND_ACC_STATIC|ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_register_class_alias("Logger", logger_ce);
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
    logger_begin();
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(logger)
{
    logger_end();
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

