YAZZ
====

What is YAZZ
------------
YAZZ is an advanced fork ~~with blackjack and hookers~~ of the [BAXTEP](http://code.google.com/p/baxtep/) project. YAZZ is a PHP security module, that can log executing of a PHP functions with all parameters.

Requirements
------------

 * php-devel
 * gcc
 * perl5 - just for yazzrep

How to install
--------------
		wget -O yazz-master.zip --no-check-certificate https://github.com/zerodotfive/yazz/archive/master.zip
		unzip yazz-master.zip
		cd yazz-master
		phpize
		./configure
		make; make install
- - -

Then edit /etc/php/php.ini and add next lines:

		extension=yazz.so;
		yazz.logfile=/var/log/yazz.log;
		yazz.functions=exec,system,shell_exec,passthru; (or other: e.g. fwrite, mysql_query)
		yazz.logformat=xml; ("plain" or "xml" - plain by default. xml with yazzrep - best for logging and searching functions calls with multiline arguments)
		yazz.maxopersize=102400; (max logged function parameter size)
- - -

in php.ini:

		auto_globals_jit --- should be off to log not initialized $_SERVER['REQUEST_URI'], $_SERVER['REMOTE_ADDR']
- - -

Then type in console:

		touch /var/log/yazz.log ; chmod 777 /var/log/yazz.log
		apachectl stop; sleep 2; apachectl start
		cp ./yazzrep /usr/bin/yazzrep
		chmod 755 /usr/bin/yazzrep
		yazzrep --help
- - -


Log samples
===========
Plaintext log
-------------

		2012-07-05 16:54:40 FUNCTION: fopen("./temp/log.dat","ab"); INFILE: /usr/home/web/test.com/www/test.php on line 39 URI: /test.php REMOTE_ADDR: 1.2.3.4
- - -

XML log
-------
		<yazzevent timestamp="2012-07-05 16:51:10" file="/usr/home/web/test.com/www/test.php" line="30" uri="test.php?l=600" remote_addr="1.2.3.4">
		        <function>mysql_query("set wait_timeout=600");</function>
		</yazzevent>
- - -


