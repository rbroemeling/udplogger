#!/usr/bin/python
# -*- coding: utf-8 -*-

import Nexopia.UDPLogger.Parse

import getopt
import logging
import multiprocessing
import os
import Queue
import re
import socket
import sys
import time
import urllib2

class ResultSummary:
	"""A simple class to keep track of our result counts, grouped by response code."""
	last_display = 0
	status = {}
	total = 0

	def __str__(self):
		s = ""
		if self.total > 0:
			s += "%d results (code[occurrences])" % self.total
			for code in self.status:
				s += " %s[%s]" % (code, self.status[code])
		return s

	def add(self, result):
		if (result not in self.status):
			self.status[result] = 0
		self.status[result] += 1
		self.total += 1

	def checkpoint(self):
		if (time.time() - self.last_display) > 30:
			self.last_display = time.time()
			print "Checkpoint: " + self

def fetch_worker(url_queue, response_queue, options):
	for url in iter(url_queue.get, "STOP"):
		req = urllib2.Request(url, None, {"User-Agent": "udploggersiege.py/" + re.sub("[^0-9]", "", "$Revision$")})
		if options["target-vhost"] is not None:
			req.add_header("Host", options["target-vhost"])
		try:
			result = urllib2.urlopen(req)
		except urllib2.HTTPError, e:
			response_queue.put(e.code, True)
		except urllib2.URLError, e:
			if str(e.args[0]).startswith("timed out"):
				response_queue.put("Timeout", True)
			else:
				response_queue.put("Error", True)
		else:
			response_queue.put(200, True)

def harvest_results(query_count, response_queue, results, block):
	while results.total < query_count:
		try:
			status = response_queue.get(False)
			results.add(status)
		except Queue.Empty:
			if not block:
				break

def main(options):
	lineno = 0
	log_data = Nexopia.UDPLogger.Parse.LogLine()
	query_count = 0
	response_queue = multiprocessing.Queue()
	results = ResultSummary()
	timestamp_delta = None
	url_queue = multiprocessing.Queue()
	
	# Start our fetch worker processes.
	for i in range(options["max-concurrent-requests"]):
		multiprocessing.Process(target=fetch_worker, args=(url_queue, response_queue, options)).start()
	
	# Iterate over stdin and queue each request that we find.
	for line in sys.stdin:
		lineno += 1
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			sys.stderr.write("skipping line #%d, could not parse data '%s': %s\n" % (lineno, line.replace("\x1e", "\\x1e"), str(e)))
			continue
		if timestamp_delta is None:
			timestamp_delta = time.time() - log_data.unix_timestamp
		if not options["flood"]:
			i = time.time() - log_data.unix_timestamp
			if (i < timestamp_delta):
				time.sleep(timestamp_delta - i)
		url_queue.put(options["target-host"] + log_data.request_url, True)
		query_count += 1
		harvest_results(query_count, response_queue, results, False)
		results.checkpoint()

	# Stop all of our fetch worker processes.
	for i in range(options["max-concurrent-requests"]):
		url_queue.put("STOP", True)

	# Ensure that we have harvested all of our results.
	harvest_results(query_count, response_queue, results, True)

	# Display our results.
	print "Run Complete: " + results
	
def parse_arguments(argv):
	options = {}
	options["debug"] = False
	options["flood"] = False
	options["max-concurrent-requests"] = 16
	options["target-host"] = None
	options["target-vhost"] = None
	options["timeout"] = 2

	try:
		opts, args = getopt.getopt(argv, "hv", ["debug", "flood", "help", "max-concurrent-requests=", "target-host=", "target-vhost=", "timeout=", "version"])
	except getopt.GetoptError, e:
		print str(e)
		usage()
		sys.exit(3)
	for o, a in opts:
		if o in ["--debug"]:
			options["debug"] = True
		elif o in ["--flood"]:
			options["flood"] = True
		elif o in ["-h", "--help"]:
			usage()
			sys.exit(0)
		elif o in ["--max-concurrent-requests"]:
			try:
				options["max-concurrent-requests"] = int(a)
				if (options["max-concurrent-requests"] <= 0):
					raise ValueError
			except (TypeError, ValueError), e:
				sys.stderr.write("invalid argument for option max-concurrent-requests: '%s'\n" % (a))
				usage()
				sys.exit(2)
		elif o in ["--target-host"]:
			options["target-host"] = a
		elif o in ["--target-vhost"]:
			options["target-vhost"] = a
		elif o in ["--timeout"]:
			try:
				options["timeout"] = int(a)
				if (options["timeout"] <= 0):
					raise ValueError
			except (TypeError, ValueError), e:
				sys.stderr.write("invalid argument for option timeout: '%s'\n" % (a))
				usage()
				sys.exit(2)
		elif o in ["-v", "--version"]:
			print "udploggersiege.py r%s" % (re.sub("[^0-9]", "", "$Revision$"))
			sys.exit(0)
		else:
			assert False, "unhandled option: " + o

	# Initialize our logging layer.
	if options["debug"]:
		logging.basicConfig(level = logging.DEBUG)
	else:
		logging.basicConfig(level = logging.INFO)

	# Make --target-host a required option.
	if options["target-host"] is None:
		sys.stderr.write("no target-host specified")
		usage()
		sys.exit(2)
	
	# Set the socket timeout as requested.
	socket.setdefaulttimeout(options["timeout"])

	return options

def usage():
	print """
Usage %s --target-host <host> [OPTIONS]

      --debug                                    display verbose debugging information
      --flood                                    do not mirror the request load by delaying between requests
  -h, --help                                     display this help and exit
      --max-concurrent-requests <num>            allow no more than <num> requests to be sent concurrently (default: 16)
      --target-host <host>                       send requests to <host> (example: http://beta.nexopia.com)
      --target-vhost <vhost>                     over-ride the default <host> header setting and set the vhost to <vhost>
      --timeout <secs>                           timeout connection requests after <secs> seconds (default: 2)
  -v, --version                                  display udploggersiege.py version and exit
""" % (sys.argv[0])

if __name__ == "__main__":
	main(parse_arguments(sys.argv[1:]))
