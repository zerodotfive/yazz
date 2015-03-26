#include "stdio.h"
#include "yazz.h"
#include <time.h>
#include <stdlib.h>

ZEND_DECLARE_MODULE_GLOBALS(yazz)

zend_module_entry yazz_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "yazz",
    NULL,
    PHP_MINIT(yazz),
    PHP_MSHUTDOWN(yazz),
    PHP_RINIT(yazz),
    PHP_RSHUTDOWN(yazz),
    PHP_MINFO(yazz),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_YAZZ_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

PHP_INI_BEGIN()
    PHP_INI_ENTRY("yazz.logfile", "/tmp/yazz.log", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("yazz.functions", "exec,system,shell_exec,passthru", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("yazz.logformat", "plain", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("yazz.maxopersize", "102400", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
PHP_INI_END()

#ifdef COMPILE_DL_YAZZ
    ZEND_GET_MODULE(yazz)
#endif

static void php_yazz_globals_ctor(zend_yazz_globals *yazz_global TSRMLS_DC)
{
    yazz_global->stolen_functions = NULL;
}

PHP_MINIT_FUNCTION(yazz)
{
#ifdef ZTS
    ts_allocate_id(&yazz_globals_id, sizeof(zend_yazz_globals), php_yazz_globals_ctor, NULL);
#else
    php_yazz_globals_ctor(&yazz_globals);
#endif

    REGISTER_INI_ENTRIES();
    YAZZ_G(logformat) = INI_STR("yazz.logformat");
    YAZZ_G(watchfunctionslist) = INI_STR("yazz.functions");
    YAZZ_G(maxopersize) = INI_STR("yazz.maxopersize");
    YAZZ_G(stolen_functions) = (HashTable *) emalloc(sizeof(HashTable));
    zend_hash_init(YAZZ_G(stolen_functions), 4, NULL, NULL, 1);

    char* watchfunctionslist = (char*) emalloc(strlen(YAZZ_G(watchfunctionslist))*sizeof(char)+2*sizeof(char));
    strcpy (watchfunctionslist, YAZZ_G(watchfunctionslist));
    strncat (watchfunctionslist,",\0",1);
    int fListOffset=0;
    int wfoffset=0;
    char wfuncname[256];
    memset(&wfuncname,0,256*sizeof(char));
    wfoffset=0;
    while(strncmp(watchfunctionslist,"\0",1)!=0){
        if (strncmp(watchfunctionslist," ",1)!=0){
            if(strncmp(watchfunctionslist,",",1)!=0){
                wfuncname[wfoffset++]=*watchfunctionslist;
            }
            else {
                if (wfoffset>2){
                    wfuncname[wfoffset]=0;
                    php_yazz_substitute_function(wfuncname);
                }
                memset(&wfuncname,0,256*sizeof(char));
                wfoffset=0;
            }
        }
        watchfunctionslist++;
    }
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(yazz)
{
    UNREGISTER_INI_ENTRIES();

    if (YAZZ_G(stolen_functions)) {
        zend_hash_destroy(YAZZ_G(stolen_functions));
        pefree(YAZZ_G(stolen_functions), 1);
        YAZZ_G(stolen_functions) = NULL;
    }

    return SUCCESS;
}

PHP_RINIT_FUNCTION(yazz)
{
    php_yazz_logger_init();

    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(yazz)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(yazz)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "YAZZ Support", "Enabled");
    php_info_print_table_header(2, "Version", PHP_YAZZ_VERSION);
    php_info_print_table_header(2, "Logfile", YAZZ_G(logfile)?YAZZ_G(logfile):"Not writable");
    php_info_print_table_header(2, "Watched functions", YAZZ_G(watchfunctionslist));
    php_info_print_table_header(2, "Log file format", YAZZ_G(logformat));
    php_info_print_table_header(2, "Max logged operand size(bytes)", YAZZ_G(maxopersize));
    php_info_print_table_end();
}

PHP_NAMED_FUNCTION(php_yazz_execution_interceptor)
{
    php_yazz_zif *fe;
    zval **exec_str;

    char *fname = (char *) get_active_function_name(TSRMLS_C);
    int ccl;
    char ParamsTMPString[256];
    memset(&ParamsTMPString,0,256*sizeof(char));
    zval **ztpar;
    zval zvalFull;
    zvalue_value zvalStr;
    char* StringParams=(char*) emalloc(sizeof(char));
    memset(StringParams,0,sizeof(char));
    char* ptzval;
    int LoggedOpSize = YAZZ_G(maxopersize);
    int ZendParamsNum = ZEND_NUM_ARGS();
    if (ZendParamsNum >= 1 && zend_get_parameters_array_ex(ZendParamsNum, &ztpar) != FAILURE) {
        int zpCounter;
        for (zpCounter=0; zpCounter<ZendParamsNum; zpCounter++){
            memset(&ParamsTMPString,0,256*sizeof(char));
            if (zpCounter!=0){
                sprintf(ParamsTMPString, "\",\"");
                StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                strcat(StringParams,ParamsTMPString);
            }
            else {
                sprintf(ParamsTMPString, " ");
                StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                strcat(StringParams,ParamsTMPString);
            }
            switch(Z_TYPE_P(ztpar[zpCounter])) {
                case IS_NULL:
                    sprintf(ParamsTMPString, "NULL");
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_LONG:
                    sprintf(ParamsTMPString, "%d", Z_LVAL_P(ztpar[zpCounter]));
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_DOUBLE:
                    sprintf(ParamsTMPString, "%lf", Z_DVAL_P(ztpar[zpCounter]));
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_STRING:
                    memcpy(&zvalFull,ztpar[zpCounter],sizeof(zval));
                    memcpy(&zvalStr,&zvalFull.value,sizeof(zvalue_value));
                    if (zvalStr.str.len <= LoggedOpSize) {
                        StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+zvalStr.str.len*sizeof(char)+sizeof(char));
                        memset(StringParams+strlen(StringParams)*sizeof(char),0,zvalStr.str.len*sizeof(char)+sizeof(char));
                        memcpy(StringParams,zvalStr.str.val,zvalStr.str.len);
                    }
                    else {
                        sprintf(ParamsTMPString, "YAZZ: too long operand to log it.");
                        StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                        memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                        strncat(StringParams,ParamsTMPString,strlen(ParamsTMPString));
                    }
                break;
                case IS_ARRAY:
                    sprintf(ParamsTMPString, "ARRAY");
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_OBJECT:
                    sprintf(ParamsTMPString, "OBJECT");
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_BOOL:
                    if(Z_LVAL_P(ztpar[zpCounter])){
                        sprintf(ParamsTMPString, "TRUE");
                        StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                        memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                        strcat(StringParams,ParamsTMPString);
                    }
                    else {
                        sprintf(ParamsTMPString, "FALSE");
                        StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                        memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                        strcat(StringParams,ParamsTMPString);
                    }
                break;
                case IS_RESOURCE:
                    sprintf(ParamsTMPString, "Resource #%d", Z_LVAL_P(ztpar[zpCounter]));
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
                case IS_CONSTANT:
                    sprintf(ParamsTMPString, "CONST");
                    StringParams=erealloc(StringParams,strlen(StringParams)*sizeof(char)+strlen(ParamsTMPString)*sizeof(char));
                    memset(StringParams+strlen(StringParams)*sizeof(char),0,strlen(ParamsTMPString)*sizeof(char)+sizeof(char));
                    strcat(StringParams,ParamsTMPString);
                break;
            }
        }
    }
    if (php_yazz_log(StringParams, fname TSRMLS_CC) == FAILURE) {
        // make fatal error if needed
        RETURN_FALSE;
    }
    if (zend_hash_find(YAZZ_G(stolen_functions), fname, strlen(fname) + 1, (void**)&fe) != FAILURE) {
        (*fe)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
}

int php_yazz_substitute_function(char *fname TSRMLS_DC)
{
    zend_internal_function *fe;
    int fname_len = strlen(fname);

    if (zend_hash_find(CG(function_table), fname, fname_len + 1, (void**)&fe) == FAILURE) {
        return FAILURE;
    }

    if (fe->type != ZEND_INTERNAL_FUNCTION) {
        return FAILURE;
    }

    if (PHP_YAZZ_HASH_ADD(YAZZ_G(stolen_functions), fname, (void*)&(fe->handler), sizeof(php_yazz_zif)) == FAILURE) {
        return FAILURE;
    }

    fe->handler = php_yazz_execution_interceptor;
    return SUCCESS;
}

void php_yazz_log_write(char *message TSRMLS_DC)
{
    if (YAZZ_G(logfile) == 0) {
        return;
    }

    php_stream *stream = php_stream_open_wrapper(YAZZ_G(logfile), "a", IGNORE_URL_WIN | ENFORCE_SAFE_MODE, NULL);

    if (stream) {
        php_stream_write(stream, message, strlen(message));
        php_stream_close(stream);
    }
}

int php_yazz_log(char *exec_str, char *fname TSRMLS_DC)
{
    char *log_str;
    char *date = (char *) emalloc(20);
    char *uri = NULL;
    char *remote_addr = NULL;
    zval **uridata;
    zval **raddrdata;
    time_t t;
    struct tm *tmp;

    // format current time
    t = time(NULL);
    tmp = localtime(&t);
    strftime(date, 20, "%F %T", tmp);

    //Set auto_global_jit=On to populate $_SERVER array
    //zend_register_auto_global("_SERVER", sizeof("_SERVER")-1, NULL TSRMLS_CC, NULL TSRMLS_CC);

    // get request uri
        if (PG(http_globals)[TRACK_VARS_SERVER] &&
            zend_hash_find(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "REQUEST_URI", sizeof("REQUEST_URI"), (void **) &uridata) == SUCCESS) {
            uri = Z_STRVAL_PP(uridata);
    }

    // get requestor ip
    if (PG(http_globals)[TRACK_VARS_SERVER] && zend_hash_find(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "REMOTE_ADDR", sizeof("REMOTE_ADDR"), (void **) &raddrdata) == SUCCESS) {
        remote_addr = Z_STRVAL_PP(raddrdata);
    }


    // create message
    if(strcmp(YAZZ_G(logformat),"plain")==0){
        spprintf(&log_str, 0, "%s FUNCTION: %s(\"%s\"); INFILE: %s on line %i URI: %s REMOTE_ADDR: %s\n",
            date,
            fname,
            exec_str,
            zend_get_executed_filename(TSRMLS_C),
            zend_get_executed_lineno(TSRMLS_C),
            uri,
            remote_addr
        );
    }
    else {
        if(strcmp(YAZZ_G(logformat),"xml")==0){
            spprintf(&log_str, 0, "<yazzevent timestamp=\"%s\" file=\"%s\" line=\"%i\" uri=\"%s\" remote_addr=\"%s\">\n\t<function>%s(\"%s\");</function>\n</yazzevent>\n",
                date,
                zend_get_executed_filename(TSRMLS_C),
                zend_get_executed_lineno(TSRMLS_C),
                uri,
                remote_addr,
                fname,
                exec_str
            );
        }
    }

    php_yazz_log_write(log_str TSRMLS_CC);

    efree(log_str);
    efree(date);

    return SUCCESS;
}

void php_yazz_logger_init()
{
    YAZZ_G(logfile) = INI_STR("yazz.logfile");
    YAZZ_G(logformat) = INI_STR("yazz.logformat");
    php_stream *stream = php_stream_open_wrapper(YAZZ_G(logfile), "a", USE_PATH, NULL);

    if (stream) {
    php_stream_close(stream);
    }
    else {
        printf ("Can't write to logfile %s\n", YAZZ_G(logfile));
    }
}
