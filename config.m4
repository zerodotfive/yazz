PHP_ARG_ENABLE(yazz, whether to enable yazz support,
[  --enable-yazz           Enable yazz support])

PHP_NEW_EXTENSION(yazz, yazz.c, $ext_shared)
