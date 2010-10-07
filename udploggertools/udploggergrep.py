#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# The MIT License (http://www.opensource.org/licenses/mit-license.php)
# 
# Copyright (c) 2010 Nexopia.com, Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import Nexopia.UDPLogger.Parse

import getopt
import re
import sys
import time

def main(options):
	log_data = Nexopia.UDPLogger.Parse.LogLine()

	lineno = 0
	for line in sys.stdin:
		lineno += 1
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			sys.stderr.write('skipping line #%d, could not parse data "%s": %s\n' % (lineno, line.replace('\x1e', '\\x1e'), str(e)))
			continue

		if not options['time-after'] is None:
			if options['time-after'] > log_data.unix_timestamp:
				continue

		if not options['time-before'] is None:
			if options['time-before'] < log_data.unix_timestamp:
				continue

		if not options['remote-ip'] is None:
			if options['remote-ip'] != log_data.remote_address:
				continue

		if not options['status'] is None:
			if options['status'] != log_data.status:
				continue

		if not options['method'] is None:
			if options['method'] != log_data.method:
				continue

		if not options['tag'] is None:
			if options['tag'] != log_data.tag:
				continue

		if not options['time-used'] is None:
			if options['time-used'] != log_data.time_used:
				continue

		if not options['content-type'] is None:
			if options['content-type'].search((log_data.content_type, "")[log_data.content_type is None]) == None:
				continue

		if not options['host'] is None:
			if options['host'].search((log_data.host, "")[log_data.host is None]) == None:
				continue

		if not options['query'] is None:
			if options['query'].search((log_data.query_string, "")[log_data.query_string is None]) == None:
				continue

		if not options['url'] is None:
			if options['url'].search((log_data.request_url, "")[log_data.request_url is None]) == None:
				continue

		if not options['nexopia-userid'] is None:
			if options['nexopia-userid'] != log_data.nexopia_userid:
				continue

		print line

def parse_arguments(argv):
	options = {}
	options['content-type'] = None
	options['host'] = None
	options['method'] = None
	options['query'] = None
	options['remote-ip'] = None
	options['status'] = None
	options['tag'] = None
	options['time-after'] = None
	options['time-before'] = None
	options['time-used'] = None
	options['url'] = None
	options['nexopia-userid'] = None

	try:
		opts, args = getopt.getopt(argv, 'hv', ['content-type=', 'help', 'host=', 'method=', 'nexopia-userid=', 'query=', 'remote-ip=', 'status=', 'tag=', 'time-after=', 'time-before=', 'time-used=', 'url=', 'version'])
	except getopt.GetoptError, e:
		print str(e)
		usage()
		sys.exit(3)
	for o, a in opts:
		if o in ['--content-type']:
			try:
				options['content-type'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option content-type: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ['-h', '--help']:
			usage()
			sys.exit(0)
		elif o in ['--host']:
			try:
				options['host'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option host: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ['--method']:
			options['method'] = a
		elif o in ['--nexopia-userid']:
			try:
				options['nexopia-userid'] = int(a)
				if options['nexopia-userid'] < 0:
					raise ValueError
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option nexopia-userid (must be integer x, where x >= 0): "%s"\n' % (a))
				usage()
				sys.exit(2)
		elif o in ['--query']:
			try:
				options['query'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option query: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ['--remote-ip']:
			options['remote-ip'] = a
		elif o in ['--status']:
			try:
				options['status'] = int(a)
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option status: "%s"\n' % (a))
				usage()
				sys.exit(2)
		elif o in ['--tag']:
			options['tag'] = a
		elif o in ['--time-after']:
			try:
				options['time-after'] = time.mktime(time.strptime(a, '%Y-%m-%d %H:%M:%S'))
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option time-after: "%s"\n' % (a))
				sys.stderr.write('date and times must be in the format "%Y-%m-%d %H:%M:%S" (i.e. "2009-10-20 15:18:17")\n')
				usage()
				sys.exit(2)
		elif o in ['--time-before']:
			try:
				options['time-before'] = time.mktime(time.strptime(a, '%Y-%m-%d %H:%M:%S'))
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option time-before: "%s"\n' % (a))
				sys.stderr.write('date and times must be in the format "%Y-%m-%d %H:%M:%S" (i.e. "2009-10-20 15:18:17")\n')
				usage()
				sys.exit(2)
		elif o in ['--time-used']:
			try:
				options['time-used'] = int(a)
				if options['time-used'] not in range(-1, 11):
					raise ValueError
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option time-used (must be integer x, where -1 <= x <= 10): "%s"\n' % (a))
				usage()
				sys.exit(2)
		elif o in ['--url']:
			try:
				options['url'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option url: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ['-v', '--version']:
			print 'udploggergrep.py r%s' % (re.sub('[^0-9]', '', '$Revision$'))
			sys.exit(0)
		else:
			assert False, 'unhandled option: ' + o
	return options

def usage():
	print '''
Usage %s [OPTIONS]

      --content-type <regexp>                    show log entries whose content-type matches <regexp>
  -h, --help                                     display this help and exit
      --host <regexp>                            show log entries whose host matches <regexp>
      --method <method>                          show log entries whose request method equals <method>
      --nexopia-userid <uid>                     show log entries whose nexopia UID equals <uid>
      --query <regexp>                           show log entries whose query string matches <regexp>
      --remote-ip <ip>                           show log entries whose remote ip matches <ip>
      --status <status code>                     show log entries whose status code equals <status code>
      --tag <tag>                                show log entries whose tag equals <tag>
      --time-after <date/time>                   show log entries that occurred at-or-after <date/time> (e.g. 2009-10-20 15:18:17)
      --time-before <date/time>                  show log entries that occurred before-or-at <date/time> (e.g. 2009-10-20 17:18:17)
      --time-used <time in seconds>              show log entries that took exactly <time in seconds> to complete
      --url <regexp>                             show log entries whose url matches <regexp>
  -v, --version                                  display udploggergrep.py version and exit
''' % (sys.argv[0])

if __name__ == '__main__':
	main(parse_arguments(sys.argv[1:]))
