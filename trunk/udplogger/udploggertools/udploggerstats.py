#!/usr/bin/python

import Nexopia.UDPLogger.Parse
import Nexopia.UDPLogger.Statistics

import getopt
import inspect
import sqlite3
import sys
import time

def main(options):
	log_data = Nexopia.UDPLogger.Parse.LogLine()

	statistics_gatherers = []
	for object in Nexopia.UDPLogger.Statistics.__dict__:
		object = getattr(Nexopia.UDPLogger.Statistics, object)
		if inspect.isclass(object):
			if hasattr(object, 'save') and hasattr(object, 'update'):
				statistics_gatherers.append(object())

	for line in sys.stdin:
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			sys.stderr.write('skipping line, could not parse data "%s": %s\n' % (line.replace('\x1e', '\\x1e'), str(e)))
			continue

		#
		# Reach into our log_data and reset the time to have zero minutes/seconds.
		# This makes the log_data have a time granularity of one hour rather than
		# one second.
		#
		log_data.unix_timestamp = time.mktime((log_data.date_time.tm_year, log_data.date_time.tm_mon, log_data.date_time.tm_mday, log_data.date_time.tm_hour, 0, 0, log_data.date_time.tm_wday, log_data.date_time.tm_yday, log_data.date_time.tm_isdst))
		log_data.date_time = time.localtime(log_data.unix_timestamp)

		for i in range (0, len(statistics_gatherers)):
			statistics_gatherers[i].update(log_data)

	for i in range (0, len(statistics_gatherers)):
		if options['verbosity'] > 0:
			print statistics_gatherers[i]
		if not options['database'] is None:
			statistics_gatherers[i].save(options['database'])

def parse_arguments(argv):
	options = {}
	options['database'] = None
	options['verbosity'] = 0

	try:
		opts, args = getopt.getopt(argv, 'd:hv', ['database=', 'help', 'verbose', 'version'])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		sys.exit(3)
	for o, a in opts:
		if o in ('-d', '--database'):
			options['database'] = sqlite3.connect(a)
		elif o in ('-h', '--help'):
			usage()
			sys.exit(0)
		elif o in ('-v', '--verbose'):
			options['verbosity'] += 1
		elif o in ('--version'):
			r = '$Revision$'
			r = r.strip(' $')
			r = r.lower()
			r = r.replace(': ', ' r')
			print 'udploggerstats.py', r
			sys.exit(0)
		else:
			assert False, 'unhandled option: ' + o
	return options

def usage():
	print '''
Usage %s [OPTIONS]

  -d, --database <db path>                       use <db path> as the sqlite data store for statistic storage
  -h, --help                                     display this help and exit
  -v, --verbose                                  display calculated statistics after run
  --version                                      display udploggerstats.py version and exit
''' % (sys.argv[0])

if __name__ == '__main__':
	main(parse_arguments(sys.argv[1:]))
