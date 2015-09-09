/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
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
zend_class_entry *ormclass_ce;
zend_fcall_info luoxin_call_user_method(zval** retval, zval* obj, char* function_name, char* paras, ...);
void pdo_query(zval* return_value, zval* pdo_obj,char* sql,int sql_len,int one TSRMLS_DC);


/* {{{ rp_orm_module_entry
 */
zend_module_entry rp_orm_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "rp_orm",
    NULL,
    PHP_MINIT(rp_orm),
    PHP_MSHUTDOWN(rp_orm),
    PHP_RINIT(rp_orm),      /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(rp_orm),  /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(rp_orm),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_RP_ORM_VERSION,
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

static PHP_METHOD(ormclass,query)
{
    zend_class_entry *ce;
    zval *obj;
    char *sql;
    int len;

    ce = Z_OBJCE_P(getThis());
    obj = zend_read_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    // obj = zend_read_static_property(ce,"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&sql,&len) == FAILURE)
    {
        RETURN_FALSE;
    }

    pdo_query(return_value,obj,sql,len,0 TSRMLS_DC);
}

/* get unique primary columnname*/
ZEND_METHOD(ormclass,getUniPri)
{
    zend_class_entry *ce;
    zval *obj;
    char *tableName;
    int len;
    char sql[100];

    ce = Z_OBJCE_P(getThis());
    obj = zend_read_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    // obj = zend_read_static_property(ce,"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&tableName,&len) == FAILURE)
    {
        RETURN_FALSE;
    }
    sprintf(sql,"SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE WHERE TABLE_NAME='%s';",tableName);
    // printf("%s",sql);
    pdo_query(return_value,obj,sql,strlen(sql),0 TSRMLS_DC);
}

/* 设置表的主键 */
ZEND_METHOD(ormclass,setPriKey)
{
    zend_class_entry *ce;
    zval *priKey;

    ce = Z_OBJCE_P(getThis());
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&priKey) == FAILURE)
    {
        RETURN_FALSE;
    }
    zend_update_property(ce,getThis(),"pri_key",sizeof("pri_key") - 1,priKey TSRMLS_CC);
    RETURN_BOOL(1);
}

/* 设置表名 */
ZEND_METHOD(ormclass,setTableName)
{
    zend_class_entry *ce;
    zval *tableName;

    ce = Z_OBJCE_P(getThis());
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&tableName) == FAILURE)
    {
        RETURN_FALSE;
    }
    zend_update_property(ce,getThis(),"table_name",sizeof("table_name") - 1,tableName TSRMLS_CC);
    RETURN_BOOL(1);
}

/* 通过主键获取数据 */
ZEND_METHOD(ormclass,findOne)
{
    zend_class_entry *ce;
    zval *obj,*table_name,*pri_key;
    char *val,*pri = NULL;
    int vlen,plen;
    char sql[100];

    ce = Z_OBJCE_P(getThis());
    obj = zend_read_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,0 TSRMLS_CC);
    table_name = zend_read_property(ce,getThis(),"table_name",sizeof("table_name") - 1,0 TSRMLS_CC);

    if(!obj || !table_name)
    {
        RETURN_FALSE;
    }
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|s",&val,&vlen,&pri,&plen) == FAILURE)
    {
        RETURN_FALSE;
    }
    if(!pri)
    {
        pri_key = zend_read_property(ce,getThis(),"pri_key",sizeof("pri_key") - 1,0 TSRMLS_CC);
        if(!pri_key)
        {
            RETURN_FALSE;
        }
        pri = Z_STRVAL_P(pri_key);
    }
    sprintf(sql,"select * from %s where %s='%s';",Z_STRVAL_P(table_name),pri,val);
    pdo_query(return_value,obj,sql,strlen(sql),1 TSRMLS_DC);
}

ZEND_METHOD(ormclass,__construct)
{
    zval *config;
    zend_class_entry *ce;
    int count,i;
    char *s,*h,*dbname,*username,*password,*port;
    char dns[100];
    zval **z_item;

    ce = Z_OBJCE_P(getThis());
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&config) == FAILURE)
    {
        RETURN_FALSE;
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

    zval *obj;
    MAKE_STD_ZVAL(obj);

    zend_class_entry *pdo_dbh_ce;
    pdo_dbh_ce = php_pdo_get_dbh_ce();
    object_init_ex(obj, pdo_dbh_ce);

    // printf("%d",167);
    luoxin_call_user_method(NULL,obj,"__construct","sss",&dns,sizeof(dns)-1,username,strlen(username),password,strlen(password));

    // zend_update_static_property(ce,"pdo_obj",sizeof("pdo_obj") - 1,obj TSRMLS_CC);

    zend_update_property(ce,getThis(),"pdo_obj",sizeof("pdo_obj") - 1,obj TSRMLS_CC);
}

zend_function_entry ormclass_method[]=
{
    PHP_ME(ormclass,query,NULL,ZEND_ACC_PUBLIC)
    PHP_ME(ormclass,getUniPri,NULL,ZEND_ACC_PUBLIC)
    PHP_ME(ormclass,findOne,NULL,ZEND_ACC_PUBLIC)
    PHP_ME(ormclass,setTableName,NULL,ZEND_ACC_PUBLIC)
    PHP_ME(ormclass,setPriKey,NULL,ZEND_ACC_PUBLIC)
    PHP_ME(ormclass,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(rp_orm)
{
    zend_class_entry orm_ce;
    INIT_CLASS_ENTRY(orm_ce,"ormclass",ormclass_method);

    ormclass_ce = zend_register_internal_class(&orm_ce TSRMLS_CC);
    zend_declare_property_null(ormclass_ce,"pdo_obj",strlen("pdo_obj"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(ormclass_ce,"table_name",strlen("table_name"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(ormclass_ce,"pri_key",strlen("pri_key"),ZEND_ACC_PUBLIC TSRMLS_CC);
    

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


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */


/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

void pdo_query(zval* return_value,zval* pdo_obj,char* sql, int sql_len,int one TSRMLS_DC){
    zend_fcall_info fci,fci2;
    fci = luoxin_call_user_method(NULL,pdo_obj,"query","s",sql,sql_len);
    if(one)
    {
        fci2 = luoxin_call_user_method(NULL,*fci.retval_ptr_ptr,"fetch","l",2);
    }
    else
    {
        fci2 = luoxin_call_user_method(NULL,*fci.retval_ptr_ptr,"fetchAll","l",2);
    }
    COPY_PZVAL_TO_ZVAL(*return_value, *fci2.retval_ptr_ptr);
    // efree(*fci.retval_ptr_ptr);
    // efree(*fci2.retval_ptr_ptr);
}

zend_fcall_info luoxin_call_user_method(zval** retval, zval* obj, char* function_name, char* paras, ...)
{
    //用于接收参数
    short int paras_count=0;
    zval*** parameters = NULL;

    long long_tmp;
    char*string_tmp;
    zval *zval_tmp;
    double dou_tmp;
    int i;
    zend_fcall_info fci;
    fci.retval_ptr_ptr = 0;

    //仅与调用有关的变量
    int retval_is_null=0;
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
                    return fci;

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
    zend_call_function(&fci, NULL TSRMLS_CC);

    //函数调用结束。
    // if(retval_is_null ==1)
    // {
    //     zval_ptr_dtor(retval);
    //     efree(retval);
    // }

    zval_ptr_dtor(&_function_name);

    // //free掉parameter及其里面的每个元素zval**，及每个元素zval**对应的zval*
    // //对于传进来的zval，不进行free，由参数调用者自行free
    if(paras_count >0)
    {
        for(i=0; i<paras_count; i++)
        {
            if(paras[i] !='z')
            {
               zval_ptr_dtor(parameters[i]);
            }
            efree(parameters[i]);
        }
        efree(parameters);
    }
    return fci;
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
