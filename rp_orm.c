/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
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

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_interfaces.h"
#include "ext/standard/info.h"
#include "Zend/zend_list.h"
#include "php_rp_orm.h"

/* If you declare any globals in php_rp_orm.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(rp_orm)
*/

/* True global resources - no need for thread safety here */
static int le_rp_orm;
zend_fcall_info fci;
zend_class_entry *ormclass_ce;
int luoxin_call_user_method(zval** retval, zval* obj, char* function_name, char* paras, ...);
void pdo_query(zval* return_value, zval* pdo_obj,char* sql,int sql_len TSRMLS_DC);

/* {{{ rp_orm_functions[]
 *
 * Every user visible function must have an entry in rp_orm_functions[].
 */
const zend_function_entry rp_orm_functions[] = {
	PHP_FE(test_pdo,	NULL)		/* For testing, remove later. */
	{NULL, NULL, NULL}	/* Must be the last line in rp_orm_functions[] */
};
/* }}} */

/* {{{ rp_orm_module_entry
 */
zend_module_entry rp_orm_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"rp_orm",
	rp_orm_functions,
	PHP_MINIT(rp_orm),
	PHP_MSHUTDOWN(rp_orm),
	PHP_RINIT(rp_orm),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(rp_orm),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(rp_orm),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_RP_ORM
ZEND_GET_MODULE(rp_orm)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("rp_orm.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_rp_orm_globals, rp_orm_globals)
    STD_PHP_INI_ENTRY("rp_orm.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_rp_orm_globals, rp_orm_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_rp_orm_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_rp_orm_init_globals(zend_rp_orm_globals *rp_orm_globals)
{
	rp_orm_globals->global_value = 0;
	rp_orm_globals->global_string = NULL;
}
*/
/* }}} */

ZEND_METHOD(ormclass,query)
{
    zend_class_entry *ce;
    zval *obj,*sql;

    ce = Z_OBJCE_P(getThis());
    obj = zend_read_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    // obj = zend_read_static_property(ce,"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&sql) == FAILURE)
    {
        RETURN_NULL();
    }
    pdo_query(return_value,obj,Z_STRVAL_P(sql),Z_STRLEN_P(sql) TSRMLS_DC);
}

ZEND_METHOD(ormclass,__construct)
{
    zval *config;
    zend_class_entry *ce;
    int count,i,port;
    char *s,*h,*dbname,*username,*password;
    char dns[100];
    zval **z_item;

    ce = Z_OBJCE_P(getThis());
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&config) == FAILURE)
    {
        RETURN_NULL();
    }

    count = zend_hash_num_elements(Z_ARRVAL_P(config));

    zend_hash_internal_pointer_reset(Z_ARRVAL_P(config)); 

    for (i = 0; i < count; i ++) {
        char* key;
        ulong idx;
        zend_hash_get_current_data(Z_ARRVAL_P(config), (void**) &z_item);
        convert_to_string_ex(z_item);
        if (zend_hash_get_current_key(Z_ARRVAL_P(config), &key, &idx, 0) == HASH_KEY_IS_STRING) {
            if(strcmp(key,"datasource") == SUCCESS)
                s = Z_STRVAL_PP(z_item);
            else if(strcmp(key,"host") == SUCCESS)
                h = Z_STRVAL_PP(z_item);
            else if(strcmp(key,"database") == SUCCESS)
                dbname = Z_STRVAL_PP(z_item);
            else if(strcmp(key,"port") == SUCCESS)
                port = Z_STRVAL_PP(z_item);
            else if(strcmp(key,"login") == SUCCESS)
                username = Z_STRVAL_PP(z_item);
            else if(strcmp(key,"password") == SUCCESS)
                password = Z_STRVAL_PP(z_item);
        } 
        zend_hash_move_forward(Z_ARRVAL_P(config));
    }

    if(!s || !h || !dbname || !username || !password || !port){
        RETURN_FALSE;
    }

    sprintf(dns,"%s:dbname=%s;host=%s;port=%s",s,dbname,h,port);

    printf("%s%s%s\n", dns,username,password);
    zval *obj;
    MAKE_STD_ZVAL(obj);

    zend_class_entry *pdo_dbh_ce;
    pdo_dbh_ce = php_pdo_get_dbh_ce();
    object_init_ex(obj, pdo_dbh_ce);


    luoxin_call_user_method(NULL,obj,"__construct","sss",&dns,sizeof(dns)-1,username,strlen(username),password,strlen(password));

    // zend_update_static_property(ce,"pdo_obj",sizeof("pdo_obj") - 1,obj TSRMLS_CC);

    zend_update_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,obj TSRMLS_CC);
}

zend_function_entry ormclass_method[]=
{
    ZEND_ME(ormclass,query,NULL,ZEND_ACC_PUBLIC)
    ZEND_ME(ormclass,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    {NULL, NULL, NULL}
};


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(rp_orm)
{
    zend_class_entry orm_ce;
    INIT_CLASS_ENTRY(orm_ce,"ormclass",ormclass_method);

    ormclass_ce = zend_register_internal_class(&orm_ce TSRMLS_CC);
    zend_declare_property_null(ormclass_ce,"pdo_obj",strlen("pdo_obj"),ZEND_ACC_PUBLIC TSRMLS_CC);


	// PHP_MINIT_FUNCTION(pdo);
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(rp_orm)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(rp_orm)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(rp_orm)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(rp_orm)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "rp_orm support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_rp_orm_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(test_pdo)
{
    zval *obj,*pobj,*arr;
    HashTable *myht;
    // uint count;
    MAKE_STD_ZVAL(obj);
    MAKE_STD_ZVAL(pobj);

    int count, i;
    zval **z_item;
    ulong index;

    zend_class_entry *pdo_dbh_ce;
    pdo_dbh_ce = php_pdo_get_dbh_ce();
    object_init_ex(obj, pdo_dbh_ce);

    luoxin_call_user_method(NULL,obj,"__construct","sss","mysql:dbname=test;host=localhost",sizeof("mysql:dbname=test;host=localhost") -1,"root",sizeof("root") - 1,"root@sn201310",sizeof("root@sn201310") -1);
    luoxin_call_user_method(NULL,obj,"query","s","select * from test",sizeof("select * from test") - 1);
    COPY_PZVAL_TO_ZVAL(*pobj,*fci.retval_ptr_ptr);
    // efree(fci);
    ALLOC_ZVAL(arr);
    while(1){
        luoxin_call_user_method(NULL,pobj,"fetch","l",PDO_FETCH_ASSOC);

        if(Z_LVAL_P(*fci.retval_ptr_ptr) == 0)
            break;
        COPY_PZVAL_TO_ZVAL(*arr, *fci.retval_ptr_ptr);
        count = zend_hash_num_elements(Z_ARRVAL_P(arr));
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(arr)); 

        for (i = 0; i < count; i ++) {
            char* key;
            ulong idx;
            zend_hash_get_current_data(Z_ARRVAL_P(arr), (void**) &z_item);
            convert_to_string_ex(z_item);
            if (zend_hash_get_current_key(Z_ARRVAL_P(arr), &key, &idx, 0) == HASH_KEY_IS_STRING) {
            // KEY爲字符串
            } else {
            }
            // 將數組中的內部指針向前移動一位
            zend_hash_move_forward(Z_ARRVAL_P(arr));
        }
    // COPY_PZVAL_TO_ZVAL(*return_value, *fci.retval_ptr_ptr);
    }
    // while(1){
    //     luoxin_call_user_method(NULL,pobj,"fetch","l",PDO_FETCH_ASSOC);
    //     COPY_PZVAL_TO_ZVAL(*arr, *fci.retval_ptr_ptr);
    //     if(Z_LVAL_P(arr) == 0)
    //         break;

    //     COPY_PZVAL_TO_ZVAL(*return_value, arr);

    //     // myht = Z_ARRVAL_P(arr);
    //     // zend_hash_internal_pointer_reset_ex(ht, &iterator);
    //     // while (zend_hash_get_current_data_ex(ht, (void **) &tmp, &iterator) == SUCCESS) {
    //     //     zend_hash_get_current_key_ex(ht, &string_key, &str_len, &num_key, 0, &iterator)
    //     // } 
    // }
    // luoxin_call_user_method(NULL,pobj,"getColumnMeta","l",0);
    // COPY_PZVAL_TO_ZVAL(*return_value, *fci.retval_ptr_ptr);
    
    // luoxin_call_user_method(NULL,obj,"call_hello","");
    if (fci.params) {
        efree(fci.params);
    }
    zval_ptr_dtor(&obj);
    return;
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

void pdo_query(zval* return_value,zval* pdo_obj,char* sql, int sql_len TSRMLS_DC){
    luoxin_call_user_method(NULL,pdo_obj,"query","s",sql,sql_len);
    COPY_PZVAL_TO_ZVAL(*return_value, *fci.retval_ptr_ptr);
}

int luoxin_call_user_method(zval** retval, zval* obj, char* function_name, char* paras, ...)
{
    //用于接收参数
    short int paras_count=0;
    zval*** parameters = NULL;

    long long_tmp;
    char*string_tmp;
    zval *zval_tmp;
    double dou_tmp;
    int i;

    //仅与调用有关的变量
    int fun_re, retval_is_null=0;
    HashTable *function_table;

    //接收参数
    paras_count = strlen(paras);
    if(paras_count >0)
    {
        parameters = (zval***)emalloc(sizeof(zval**) * paras_count);

        va_list ap;
        va_start(ap,paras);

        for(i=0; i<paras_count; i++)
        {
            parameters[i] = (zval**)emalloc(sizeof(zval*));

            switch(paras[i])
            {
                case's':
                    MAKE_STD_ZVAL(*parameters[i]);
                    string_tmp = va_arg(ap, char*);
                    long_tmp = va_arg(ap, long);
                    ZVAL_STRINGL(*parameters[i], string_tmp, long_tmp, 1);
                    break;

                case'l':
                    MAKE_STD_ZVAL(*parameters[i]);
                    long_tmp = va_arg(ap, long);
                    ZVAL_LONG(*parameters[i], long_tmp);
                    break;

                case'd':
                    MAKE_STD_ZVAL(*parameters[i]);
                    dou_tmp = va_arg(ap, double);
                    ZVAL_DOUBLE(*parameters[i], dou_tmp);
                    break;

                case'n':
                    MAKE_STD_ZVAL(*parameters[i]);
                    ZVAL_NULL(*parameters[i]);
                    break;

                case'z':
                    zval_tmp = va_arg(ap, zval*);
                    *parameters[i] = zval_tmp;
                    break;

                case'b':
                    MAKE_STD_ZVAL(*parameters[i]);
                    ZVAL_BOOL(*parameters[i], (int)va_arg(ap, int));
                    break;
                default:
                    zend_error(E_ERROR, "Unsupported type:%c in walu_call_user_function", paras[i]);
                    return 0;

            }
        }

       va_end(ap);
    }

    //构造参数执行call_user_function_ex
    zval *_function_name;
    MAKE_STD_ZVAL(_function_name);
    ZVAL_STRINGL(_function_name,function_name, strlen(function_name), 1);


    if(retval == NULL)
    {
        retval_is_null =1;
        retval = (zval**)emalloc(sizeof(zval*));
    }

    //开始函数调用
    if(obj)
    {
        function_table =&Z_OBJCE_P(obj)->function_table;
    }
    else
    {
        function_table = (CG(function_table));
    }

    // zend_fcall_info fci;
    fci.size =sizeof(fci);
    fci.function_table = function_table;
    fci.object_ptr = obj ? obj : NULL;
    fci.function_name = _function_name;
    fci.retval_ptr_ptr = retval;
    fci.param_count = paras_count;
    fci.params = parameters;
    fci.no_separation =1;
    fci.symbol_table = NULL;
    fun_re = zend_call_function(&fci, NULL TSRMLS_CC);

    //函数调用结束。
    // if(retval_is_null ==1)
    // {
    //     zval_ptr_dtor(retval);
    //             efree(retval);
    // }

    // zval_ptr_dtor(&_function_name);

    // //free掉parameter及其里面的每个元素zval**，及每个元素zval**对应的zval*
    // //对于传进来的zval，不进行free，由参数调用者自行free
    // if(paras_count >0)
    // {
    //     for(i=0; i<paras_count; i++)
    //     {
    //         if(paras[i] !='z')
    //         {
    //            zval_ptr_dtor(parameters[i]);
    //         }
    //         efree(parameters[i]);
    //     }
    //     efree(parameters);
    // }
    return fun_re;
}
