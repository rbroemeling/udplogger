#!/usr/bin/python

import Nexopia.UDPLogger.Parse

import getopt
import re
import sys

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

		if not options['status'] is None:
			if options['status'] != log_data.status:
				continue

		if not options['query'] is None:
			if options['query'].search(log_data.query_string) == None:
				continue

		if not options['url'] is None:
			if options['url'].search(log_data.request_url) == None:
				continue

		print line

def parse_arguments(argv):
	options = {}
	options['query'] = None
	options['status'] = None
	options['url'] = None

	try:
		opts, args = getopt.getopt(argv, 'hq:s:u:v', ['help', 'query=', 'status=', 'url=', 'version'])
	except getopt.GetoptError, e:
		print str(e)
		usage()
		sys.exit(3)
	for o, a in opts:
		if o in ('-h', '--help'):
			usage()
			sys.exit(0)
		elif o in ('-q', '--query'):
			try:
				options['query'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option query: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ('-s', '--status'):
			try:
				options['status'] = int(a)
			except (TypeError, ValueError), e:
				sys.stderr.write('invalid argument for option status: "%s"\n' % (a))
				usage()
				sys.exit(2)
		elif o in ('-u', '--url'):
			try:
				options['url'] = re.compile(a)
			except Exception, e:
				sys.stderr.write('invalid regular expression given for option url: "%s" (%s)\n' % ((a), str(e)))
				usage()
				sys.exit(2)
		elif o in ('-v', '--version'):
			r = '$Revision$'
			r = r.strip(' $')
			r = r.lower()
			r = r.replace(': ', ' r')
			print 'udploggergrep.py', r
			sys.exit(0)
		else:
			assert False, 'unhandled option: ' + o
	return options

def usage():
	print '''
Usage %s [OPTIONS]

  -h, --help                                     display this help and exit
  -q, --query <regexp>                           show log entries whose query string matches <regexp>
  -s, --status <status code>                     show log entries whose status code equals <status code>
  -u, --url <regexp>                             show log entries whose url matches <regexp>
  -v, --version                                  display udploggergrep.py version and exit
''' % (sys.argv[0])

if __name__ == '__main__':
	main(parse_arguments(sys.argv[1:]))
