#!/usr/bin/python
# -*- coding: utf-8 -*-

__version__ = "$Revision$"

import Nexopia.UDPLogger.Parse

import logging
import multiprocessing
import optparse
import os
import Queue
import re
import socket
import sys
import time
import urllib2


class ResultSummary:
	"""A simple class to keep track of our result counts, grouped by response code."""
	checkpoint_time = 30
	last_display = 0
	status = {}
	total = 0

	def __init__(self, options):
		self.checkpoint_time = options.checkpoint

	def __str__(self):
		s = ""
		if self.total > 0:
			s += "%d results.  Breakdown (code[occurrences]):" % self.total
			for code in self.status:
				s += " %s[%s]" % (code, self.status[code])
		return s

	def add(self, result):
		if (result not in self.status):
			self.status[result] = 0
		self.status[result] += 1
		self.total += 1

	def checkpoint(self):
		if (time.time() - self.last_display) > self.checkpoint_time:
			self.last_display = time.time()
			s = str(self)
			if s:
				print "Checkpoint: " + s


def fetch_worker(url_queue, response_queue, options):
	for url in iter(url_queue.get, "STOP"):
		req = urllib2.Request(url, None, {"User-Agent": "udploggermirror.py/" + re.sub("[^0-9]", "", __version__)})
		if options.vhost is not None:
			req.add_header("Host", options.vhost)
		try:
			result = urllib2.urlopen(req)
		except urllib2.HTTPError, e:
			response_queue.put(e.code, True)
		except urllib2.URLError, e:
			if str(e.args[0]).startswith("timed out"):
				response_queue.put("Timeout", True)
			else:
				logging.exception("Unhandled URLError fetching %s", url)
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
	results = ResultSummary(options)
	timestamp_delta = None
	url_queue = multiprocessing.Queue()
	
	# Start our fetch worker processes.
	for i in range(options.concurrency):
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
		if not options.flood:
			i = time.time() - log_data.unix_timestamp
			if (i < timestamp_delta):
				time.sleep(timestamp_delta - i)
		url_queue.put(options.host + log_data.request_url, True)
		query_count += 1
		harvest_results(query_count, response_queue, results, False)
		results.checkpoint()

	# Stop all of our fetch worker processes.
	for i in range(options.concurrency):
		url_queue.put("STOP", True)

	# Ensure that we have harvested all of our results.
	harvest_results(query_count, response_queue, results, True)

	# Display our results.
	print "Run Complete: " + str(results)


def parse_arguments():
	parser = optparse.OptionParser(
			usage="%prog [options] --target-host <host>",
			version="%prog r" + re.sub("[^0-9]", "", __version__)
		)
	parser.add_option(
		"--checkpoint",
		default=30,
		help="set the length of time (in seconds) between display of checkpoints [default: %default]",
		type="int"
	)
	parser.add_option(
		"--debug",
		action="store_true",
		default=False,
		help="enable display of verbose debugging information"
	)
	parser.add_option(
		"--flood",
		action="store_true",
		default=False,
		help="disable using a delay between queueing requests to mirror request load"
	)
	parser.add_option(
		"--max-concurrent-requests",
		default=16,
		dest="concurrency",
		help="set the maximum number of requests to be sent concurrently [default: %default]",
		type="int"
	)
	parser.add_option(
		"--target-host",
		dest="host",
		help="the host to send requests to"
	)
	parser.add_option(
		"--target-vhost",
		dest="vhost",
		help="override the default vhost HTTP header (which is based on the value of HOST)"
	)
	parser.add_option(
		"--timeout",
		default=2,
		help="socket timeout to be used for connection requests [default: %default]",
		type="int"
	)

	(options, args) = parser.parse_args()
	
	# Do final checks and additional verification of parsed values.
	if not options.host:
		parser.error("option --target-host: required")
	if options.timeout <= 0:
		parser.error("option --timeout: must be larger than zero")
	if options.checkpoint <= 0:
		parser.error("option --checkpoint: must be larger than zero")
	if options.concurrency <= 0:
		parser.error("option --max-concurrent-requests: must be larger than zero")
	
	return options

if __name__ == "__main__":
	options = parse_arguments()
	
	# Initialize our logging layer.
	loglevel = logging.INFO
	if options.debug:
		loglevel = logging.DEBUG
	logging.basicConfig(datefmt = "%d %b %Y %H:%M:%S", format = "%(asctime)s %(levelname)-8s %(message)s", level = loglevel)
	
	# Set the socket timeout as requested.
	socket.setdefaulttimeout(options.timeout)
	
	main(options)
