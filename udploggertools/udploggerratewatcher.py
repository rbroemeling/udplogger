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

__version__ = "$Revision$"

import Nexopia.UDPLogger.Parse

import bisect
import logging
import optparse
import re
import shlex
import subprocess
import sys

class SlidingWindow:
	aggregate = {}
	window = {}
	window_timestamps = []
	window_size = 0
	
	def __init__(self, window_size):
		self.window_size = window_size

	def add(self, timestamp, key, value):
		if timestamp not in self.window:
			self.window[timestamp] = {}
			bisect.insort(self.window_timestamps, timestamp)
			self.expire()
		if key not in self.window[timestamp]:
			self.window[timestamp][key] = 0
		self.window[timestamp][key] = self.window[timestamp][key] + value
		if key not in self.aggregate:
			self.aggregate[key] = 0
		self.aggregate[key] = self.aggregate[key] + value 

	def expire(self):
		while (len(self.window_timestamps) > 0) and (self.window_timestamps[0] < (self.window_timestamps[len(self.window_timestamps)-1] - self.window_size)):
			expired_timestamp = self.window_timestamps.pop(0)
			for key, value in self.window[expired_timestamp].iteritems():
				assert self.aggregate[key] >= value
				self.aggregate[key] = self.aggregate[key] - value
				if self.aggregate[key] == 0:
					del self.aggregate[key]
			del self.window[expired_timestamp]
		assert len(self.window_timestamps) == len(self.window)
		assert len(self.window_timestamps) <= (self.window_size + 1)

	def fetch_keys_above(self, limit):
		keys_above = []
		for key, value in self.aggregate.iteritems():
			if value > limit:
				keys_above.append(key)
		return keys_above

def execute_command(command, log_data):
	command = command.replace("$UID$", str(log_data.nexopia_userid))
	command = command.replace("$IP$", log_data.remote_address)
	args = shlex.split(command)
	logging.debug("executing command: %s", str(args))
	return subprocess.call(args)

def main(options):
	log_data = Nexopia.UDPLogger.Parse.LogLine()
	ip_sw = SlidingWindow(options.window_size)
	uid_sw = SlidingWindow(options.window_size)

	lineno = 0
	if options.repeat_command:
		reported = None
	else:
		reported = set([])
	for line in sys.stdin:
		lineno += 1
		line = line.rstrip()
		try:
			log_data.parse(line)
		except Exception, e:
			logging.error('skipping line #%d, could not parse data "%s": %s\n' % (lineno, line.replace('\x1e', '\\x1e'), str(e)))
			continue

		if options.remote_ip and log_data.remote_address is not None:
			ip_sw.add(log_data.unix_timestamp, log_data.remote_address, 1)
			for ip in ip_sw.fetch_keys_above(options.rate):
				if reported is not None:
					if ip in reported:
						continue
					else:
						reported = reported | set([ip])				
				if ip in options.whitelist:
					logging.debug('bypassing rate-limit for whitelisted remote ip address %s' % (ip))
					continue
				else:
					logging.info('remote ip address rate-limit triggered for %s' % (ip))
					if options.command:
						execute_command(options.command, log_data)

		if options.nexopia_userid and log_data.nexopia_userid is not None:
			uid_sw.add(log_data.unix_timestamp, str(log_data.nexopia_userid), 1)
			for uid in uid_sw.fetch_keys_above(options.rate):
				if reported is not None:
					if uid in reported:
						continue
					else:
						reported = reported | set([uid])
				if uid in options.whitelist:
					logging.debug('bypassing rate-limit for whitelisted nexopia user id %s' % (uid))
					continue
				else:
					logging.info('nexopia user id rate-limit triggered for %s' % (uid))
					if options.command:
						execute_command(options.command, log_data)

def parse_arguments():
	"""
	Parse command-line arguments and setup an optparse object specifying
	the settings for this application to use.
	"""
	parser = optparse.OptionParser(
		usage="%prog [options] --command=COMMAND (--nexopia-userid|--remote-ip)",
		version="%prog r" + re.sub("[^0-9]", "", __version__)
	)
	parser.add_option(
		"--command",
		help="execute this command when the rate limit is exceeded (replacements available: $UID$ and $IP$)"
	)
	parser.add_option(
		"--debug",
		action="store_true",
		default=False,
		help="enable display of verbose debugging information"
	)
	parser.add_option(
		"--nexopia-userid",
		action="store_true",
		default=False,
		dest="nexopia_userid",
		help="rate-limit based on aggregation by nexopia user id"
	)
	parser.add_option(
		"--rate",
		default=20,
		help="trigger the rate-limit if the aggregated data shows more than this many hits within a WINDOW_SIZE period (measured in seconds)",
		type="int"
	)
	parser.add_option(
		"--remote-ip",
		action="store_true",
		default=False,
		dest="remote_ip",
		help="rate-limit based on aggregation by remote ip address"
	)
	parser.add_option(
		"--repeat-command",
		action="store_true",
		default=False,
		dest="repeat_command",
		help="trigger the command for EACH request that exceeds the rate-limit, rather than only once per data aggregation key"
	)
	parser.add_option(
		"--whitelist",
		action="append",
		help="whitelist an aggregation key (remote ip address or nexopia user id) so that it will not trigger COMMAND"
	)
	parser.add_option(
		"--window-size",
		default=60,
		dest="window_size",
		help="trigger the rate-limit if the aggregated data shows more than RATE hits within this many seconds",
		type="int"
	)
	
	(options, args) = parser.parse_args()
	
	if options.rate <= 0:
		parser.error("option --rate: must be larger than zero")
	if options.window_size <= 0:
		parser.error("option --window-size: must be larger than zero")
	if not options.nexopia_userid and not options.remote_ip:
		parser.error("must aggregate over at least one identifier, use either --nexopia-userid or --remote-ip (or both)")
	if not options.whitelist:
		options.whitelist = []
	options.whitelist = set(options.whitelist)

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
	
	main(options)
