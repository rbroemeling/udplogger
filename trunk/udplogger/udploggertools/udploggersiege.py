#!/usr/bin/python
# -*- coding: utf-8 -*-

import Nexopia.UDPLogger.Parse

import getopt
from multiprocessing import Process, Queue
import os
import re
import sys
import time
import urllib2

def fetch(url, vhost = None):
	print url
	req = urllib2.Request(url, None, {'User-Agent': 'udploggersiege.py/' + re.sub('[^0-9]', '', '$Revision$')})
	if vhost is not None:
		req.add_header('Host', vhost)
	try:
		result = urllib2.urlopen(req)
	except urllib2.HTTPError, e:
		return e.code
	except urllib2.URLError, e:
		return None
	else:
		return 200

def fetch_worker(url_queue, response_queue, options):
	for url in iter(url_queue.get, 'STOP'):
		result = fetch(url, options['target-vhost'])
		response_queue.put(result)

def main(options):
	lineno = 0
	log_data = Nexopia.UDPLogger.Parse.LogLine()
	response_queue = Queue()
	timestamp_delta = None
	url_queue = Queue()
	
	
	# Start our fetch worker processes.
	for i in range(options['max-concurrent-requests']):
		Process(target=fetch_worker, args=(url_queue, response_queue, options)).start()
	
	# Iterate over stdin and queue each request that we find.
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
		url_queue.put(options['target-host'] + log_data.request_url)
	
	# Stop all of our fetch worker processes.
	for i in range(options['max-concurrent-requests']):
		url_queue.put('STOP')

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
			print 'udploggersiege.py r%s' % (re.sub('[^0-9]', '', '$Revision$'))
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
      --target-host <host>                       send requests to <host> (example: 'http://beta.nexopia.com')
      --target-vhost <vhost>                     over-ride the default <host> header setting and set the vhost to <vhost>
  -v, --version                                  display udploggersiege.py version and exit
''' % (sys.argv[0])

if __name__ == '__main__':
	main(parse_arguments(sys.argv[1:]))
