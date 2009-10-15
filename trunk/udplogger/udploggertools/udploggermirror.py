#!/usr/bin/python
# -*- coding: utf-8 -*-

__version__ = "$Revision$"

import Nexopia.UDPLogger.Parse

import datetime
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
import urlparse


class ResultSummary:
	"""
	A simple class to keep track of summary data about our results and
	ease standardized display of that summary data to the user.
	"""
	checkpoint_time = 30
	last_checkpoint_timestamp = 0
	last_checkpoint_total = 0
	status = {}
	total = 0

	def __init__(self, options):
		self.checkpoint_time = options.checkpoint
		self.last_checkpoint_timestamp = time.time()

	def __str__(self):
		s = ""
		if self.total > 0:
			s += "%d results total - breakdown (code[occurrences]):" % self.total
			for code in self.status:
				s += " %s[%s]" % (code, self.status[code])
		return s

	def add(self, result):
		"""
		Add a given result to our summary data.
		
		Keyword arguments:
		result -- the result to add to our summary data
		"""
		if result not in self.status:
			self.status[result] = 0
		self.status[result] += 1
		self.total += 1

	def checkpoint(self):
		"""
		Display a checkpoint of the current summary data every
		checkpoint_time seconds.
		"""
		if (time.time() - self.last_checkpoint_timestamp) > self.checkpoint_time:
			delta = self.total - self.last_checkpoint_total
			rate = 0
			if self.total > 0:
				rate = delta / (time.time() - self.last_checkpoint_timestamp)
				checkpoint_str = str(self)
			else:
				checkpoint_str = "no results yet"
			print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "checkpoint (%+3d results, %02.3f/sec): %s" % (delta, rate, checkpoint_str)
			self.last_checkpoint_timestamp = time.time()
			self.last_checkpoint_total = self.total


def fetch_worker(url_queue, response_queue, vhost = None):
	"""
	Perform work: fetch URLs from an input queue, attempt to fetch the target
	of the URL, and save the resultant status codes into a response queue.
	Work will continue until the "STOP" element is encountered in the input
	queue.
	
	Keyword arguments:
	url_queue -- input queue of URLs to be fetched
	response_queue -- output queue of response codes
	vhost -- over-ride vhost ('Host:' header) to use during requests
	"""
	for url in iter(url_queue.get, "STOP"):
		req = urllib2.Request(url, None, {"User-Agent": "udploggermirror.py/" + re.sub("[^0-9]", "", __version__)})
		if vhost is not None:
			req.add_header("Host", vhost)
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


def harvest_results(max, result_queue, result_summary, block):
	"""
	Harvest results from an input queue and use them to update our summary.
	
	Keyword arguments:
	max -- harvest at most this number of results from the input queue
	result_queue -- the input queue to harvest results from
	result_summary -- the ResultSummary object to be updated
	block -- whether to block waiting for more until we have at least max results
	"""
	while result_summary.total < max:
		try:
			status = result_queue.get(False)
			result_summary.add(status)
		except Queue.Empty:
			if not block:
				break
			else:
				time.sleep(1)


def main(options):
	"""
	Read udplogger lines from stdin and assign the URLs to a pool of
	URL-fetch workers, displaying response summary information as the
	run continues.  If options.flood is enabled, dispatch requests as fast
	as possible.  If not, then attempt to mimic the query load level by
	dispatching requests at approximately the same rate as they were logged.
	
	Keyword arguments:
	options -- optparse options object specifying the settings to use
	"""
	dispatched_count = 0
	lineno = 0
	log_data = Nexopia.UDPLogger.Parse.LogLine()
	response_queue = multiprocessing.Queue()
	result_summary = ResultSummary(options)
	timestamp_delta = None
	url_queue = multiprocessing.Queue()
	
	# Start our fetch worker processes.
	for i in range(options.concurrency):
		multiprocessing.Process(target=fetch_worker, args=(url_queue, response_queue, options.vhost)).start()
	
	# Iterate over stdin and queue each request that we find, harvesting
	# whatever results are available after each line (non-blocking) and
	# displaying our checkpoint summary whenever necessary.
	for line in sys.stdin:
		lineno += 1
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			logging.error("[line %d] could not parse data '%s': %s", lineno, line.replace("\x1e", "\\x1e"), str(e))
			continue
		if timestamp_delta is None:
			timestamp_delta = time.time() - log_data.unix_timestamp
		if not options.flood:
			# If options.flood is not enabled, attempt to maintain
			# our time offset from the logged data at exactly
			# timestamp_delta seconds.
			i = time.time() - log_data.unix_timestamp
			if i < timestamp_delta:
				time.sleep(timestamp_delta - i)
		url_queue.put(options.host + log_data.request_url, True)
		dispatched_count += 1
		harvest_results(dispatched_count, response_queue, result_summary, False)
		result_summary.checkpoint()

	# Stop all of our fetch worker processes.
	for i in range(options.concurrency):
		url_queue.put("STOP", True)

	# Ensure that we have harvested all of our results.
	harvest_results(dispatched_count, response_queue, result_summary, True)

	print "Run Complete: " + str(result_summary)


def parse_arguments():
	"""
	Parse command-line arguments and setup a optparse object specifying
	the settings for this application to use.
	"""
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
	
	# Do final checks and additional/user verification of parsed values.
	if not options.host:
		parser.error("option --target-host: required")
	else:
		if options.host.find('://') == -1:
			options.host = "http://%s" % options.host
		host_parsed = urlparse.urlparse(options.host)
		options.host = "%s://%s" % (host_parsed[0], host_parsed[1])
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
	del loglevel

	logging.debug("options: %s", str(options))
	
	# Set the socket timeout as requested.
	socket.setdefaulttimeout(options.timeout)
	
	main(options)
