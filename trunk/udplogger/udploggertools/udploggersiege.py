#!/usr/bin/python
# -*- coding: utf-8 -*-

import Nexopia.UDPLogger.Parse

import getopt
import sys
import time

def main(options):
	log_data = Nexopia.UDPLogger.Parse.LogLine()

	timestamp_delta = None
	lineno = 0
	for line in sys.stdin:
		lineno += 1
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			sys.stderr.write('skipping line #%d, could not parse data "%s": %s\n' % (lineno, line.replace('\x1e', '\\x1e'), str(e)))
			continue
		if timestamp_delta is None:
			timestamp_delta = time.time() - log_data.unix_timestamp
		if options['flood'] is None:
			i = time.time() - log_data.unix_timestamp
			if (i < timestamp_delta):
				time.sleep(timestamp_delta - i)
		print log_data.request_url

def parse_arguments(argv):
	options = {}
	options['flood'] = None
	options['max-concurrent-requests'] = 128
	options['target-host'] = None
	options['target-vhost'] = None

	try:
		opts, args = getopt.getopt(argv, 'hv', ['flood', 'help', 'max-concurrent-requests=', 'target-host=', 'target-vhost=', 'version'])
	except getopt.GetoptError, e:
		print str(e)
		usage()
		sys.exit(3)
	for o, a in opts:
		if o in ['--flood']:
			options['flood'] = 1
		elif o in ['-h', '--help']:
			usage()
			sys.exit(0)
		elif o in ['--max-concurrent-requests']:
			try:
				options['max-concurrent-requests'] = int(a)
				if (options['max-concurrent-requests'] <= 0):
					raise ValueError
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option max-concurrent-requests: "%s"\n' % (a))
				usage()
				sys.exit(2)
		elif o in ['--target-host']:
			options['target-host'] = a
		elif o in ['--target-vhost']:
			options['target-vhost'] = a
		elif o in ['-v', '--version']:
			r = '$Revision$'
			r = r.strip(' $')
			r = r.lower()
			r = r.replace(': ', ' r')
			print 'udploggersiege.py', r
			sys.exit(0)
		else:
			print "in assert"
			assert False, 'unhandled option: ' + o
	if options['target-host'] is None:
		sys.stderr.write('no target-host specified')
		usage()
		sys.exit(2)
	if options['target-vhost'] is None:
		options['target-vhost'] = options['target-host']
	return options

def usage():
	print '''
Usage %s --target-host <host> [OPTIONS]

      --flood                                    do not mirror the request load by delaying between requests
  -h, --help                                     display this help and exit
      --max-concurrent-requests <num>            allow no more than <num> requests to be sent concurrently (default: 128)
      --target-host <host>                       send requests to <host>
      --target-vhost <vhost>                     over-ride the default <host> setting and set the vhost to <vhost>
  -v, --version                                  display udploggersiege.py version and exit
''' % (sys.argv[0])

if __name__ == '__main__':
	main(parse_arguments(sys.argv[1:]))
