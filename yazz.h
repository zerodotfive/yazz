#ifndef PHP_YAZZ_H
#define PHP_YAZZ_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "main/SAPI.h"

#define PHP_YAZZ_VERSION	"0.9.1"

typedef void (*php_yazz_zif)(INTERNAL_FUNCTION_PARAMETERS);

extern zend_module_entry yazz_module_entry;
#define phpext_yazz_ptr &yazz_module_entry

PHP_MINIT_FUNCTION(yazz);
PHP_MSHUTDOWN_FUNCTION(yazz);
PHP_RINIT_FUNCTION(yazz);
PHP_RSHUTDOWN_FUNCTION(yazz);
PHP_MINFO_FUNCTION(yazz);

ZEND_BEGIN_MODULE_GLOBALS(yazz)
	HashTable *stolen_functions;
	char *logfile;
	char *watchfunctionslist;
	char *logformat;
	char *maxopersize;
ZEND_END_MODULE_GLOBALS(yazz)

extern ZEND_DECLARE_MODULE_GLOBALS(yazz);

// from core
int php_yazz_steal_function(char * TSRMLS_DC);
//int php_yazz_restore_functions(void *fe, int num_args, va_list args, zend_hash_key *hash_key);

// from logger
void php_yazz_logger_init();
int php_yazz_log(char *exec_str, char *fname TSRMLS_DC);

#ifdef ZTS
#define		YAZZ_G(v)		TSRMG(yazz_globals_id, zend_yazz_globals *, v)
#define YAZZ_TSRMLS_C		TSRMLS_C
#else
#define		YAZZ_G(v)		(yazz_globals.v)
#define YAZZ_TSRMLS_C		NULL
#endif

#if defined(ZEND_ENGINE_2) && !defined(zend_hash_add_or_update)
/* Why doesn't ZE2 define this? */
#define zend_hash_add_or_update(ht, arKey, nKeyLength, pData, pDataSize, pDest, flag) \
        _zend_hash_add_or_update((ht), (arKey), (nKeyLength), (pData), (pDataSize), (pDest), (flag) ZEND_FILE_LINE_CC)
#endif

#if PHP_MAJOR_VERSION >= 6
#define PHP_YAZZ_DECL_STRING_PARAM(param)		void *param; int32_t param##_len; zend_uchar param##_type;
#define PHP_YAZZ_STRING_SPEC					"t"
#define PHP_YAZZ_STRING_PARAM(param)			&param, &param##_len, &param##_type
#define PHP_YAZZ_STRING_LEN(param,addtl)		(param##_type == IS_UNICODE ? UBYTES(param##_len + (addtl)) : (param##_len + (addtl)))
#define PHP_YAZZ_STRING_TYPE(param)			(param##_type)
#define PHP_YAZZ_HASH_FIND(hash,param,ppvar)	zend_u_hash_find(hash, param##_type, (UChar *)param, param##_len + 1, (void**)ppvar)
#define PHP_YAZZ_HASH_EXISTS(hash,param)		zend_u_hash_exists(hash, param##_type, (UChar *)param, param##_len + 1)
#define PHP_YAZZ_HASH_KEY(hash_key)			((hash_key)->type == HASH_KEY_IS_UNICODE ? (hash_key)->u.unicode : (hash_key)->u.string)
#define PHP_YAZZ_HASH_KEYLEN(hash_key)		((hash_key)->type == HASH_KEY_IS_UNICODE ? UBYTES((hash_key)->nKeyLength) : (hash_key)->nKeyLength)
#define PHP_YAZZ_HASH_ADD(hash, param, el, el_size)	zend_u_hash_add(hash, IS_STRING, (UChar *)param, param##_len + 1, el, el_size, NULL)

#elif PHP_MAJOR_VERSION >= 5
#define PHP_YAZZ_DECL_STRING_PARAM(p)			char *p; int p##_len;
#define PHP_YAZZ_STRING_SPEC					"s"
#define PHP_YAZZ_STRING_PARAM(p)				&p, &p##_len
#define PHP_YAZZ_STRING_LEN(param,addtl)		(param##_len + (addtl))
#define PHP_YAZZ_STRING_TYPE(param)			IS_STRING
#define PHP_YAZZ_HASH_FIND(hash,param,ppvar)	zend_hash_find(hash, param, param##_len + 1, (void**)ppvar)
#define PHP_YAZZ_HASH_EXISTS(hash,param)		zend_hash_exists(hash, param##_type, param, param##_len + 1)
#define PHP_YAZZ_HASH_KEY(hash_key)			((hash_key)->arKey)
#define PHP_YAZZ_HASH_KEYLEN(hash_key)		((hash_key)->nKeyLength)
#define PHP_YAZZ_HASH_ADD(hash, param, el, el_size)	zend_hash_add(hash, param, param##_len + 1, el, el_size, NULL)

#else /* PHP4 */
#define PHP_YAZZ_DECL_STRING_PARAM(p)			char *p; int p##_len;
#define PHP_YAZZ_STRING_SPEC					"s"
#define PHP_YAZZ_STRING_PARAM(p)				&p, &p##_len
#define PHP_YAZZ_STRING_LEN(param,addtl)		(param##_len + (addtl))
#define PHP_YAZZ_STRING_TYPE(param)			IS_STRING
#define PHP_YAZZ_HASH_FIND(hash,param,ppvar)	zend_hash_find(hash, param, param##_len + 1, (void**)ppvar)
#define PHP_YAZZ_HASH_EXISTS(hash,param)		zend_hash_exists(hash, param##_type, param, param##_len + 1)
#define PHP_YAZZ_HASH_KEY(hash_key)			((hash_key)->arKey)
#define PHP_YAZZ_HASH_KEYLEN(hash_key)		((hash_key)->nKeyLength)
#define PHP_YAZZ_HASH_ADD(hash, param, el, el_size)	zend_hash_add(hash, param, param##_len + 1, el, el_size, NULL)
#define zend_function_dtor						destroy_zend_function

#endif /* Version Agnosticism */

#endif	/* PHP_YAZZ_H */
