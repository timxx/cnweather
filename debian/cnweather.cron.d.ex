#
# Regular cron jobs for the cnweather package
#
0 4	* * *	root	[ -x /usr/bin/cnweather_maintenance ] && /usr/bin/cnweather_maintenance
