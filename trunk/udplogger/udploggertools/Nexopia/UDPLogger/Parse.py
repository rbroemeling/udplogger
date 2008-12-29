#!/usr/bin/python

import sys
import time

# On any call to .parse(), each class must fully initialize all the fields that it has authority over.

class LogLine_v1:
	def __str__(self):
		keys = self.__dict__.keys()
		keys.sort()
		s = 'raw => ' + str(self.raw.split('\x1e')) + '\n'
		for i in keys:
			if i != 'raw':
				if isinstance(self.__dict__[i], basestring):
					s += i + ' => "' + self.__dict__[i] + '"\n'
				else:
					s += i + ' => ' + str(self.__dict__[i]) + '\n'
		return s

	def parse(self, fields):
		self.parse_datetime(fields[0])
		self.parse_source(fields[1])
		self.parse_serial(fields[2])
		self.parse_tag(fields[3])
		self.parse_method(fields[4])
		self.parse_status(fields[5])
		self.parse_body_size(fields[6])
		self.parse_bytes_incoming(fields[7])
		self.parse_bytes_outgoing(fields[8])
		self.parse_time_used(fields[9])
		self.parse_connection_status(fields[10])
		self.parse_request_url(fields[11])
		self.parse_query_string(fields[12])
		self.parse_remote_address(fields[13])
		self.parse_user_agent(fields[14])
		self.parse_forwarded_for(fields[15])
		self.parse_referer(fields[16])
		self.parse_nexopia_userid(fields[17])
		self.parse_nexopia_userage(fields[18])
		self.parse_nexopia_usersex(fields[19])
		self.parse_nexopia_userlocation(fields[20])
		self.parse_nexopia_usertype(fields[21])

	def parse_body_size(self, field):
		if field != '-':
			try:
				self.body_size = int(field)
			except ValueError:
				self.body_size = None
		else:
			self.body_size = 0

	def parse_bytes_incoming(self, field):
		if field != '-':
			try:
				self.bytes_incoming = int(field)
			except ValueError:
				self.bytes_incoming = None
		else:
			self.bytes_incoming = 0

	def parse_bytes_outgoing(self, field):
		if field != '-':
			try:
				self.bytes_outgoing = int(field)
			except ValueError:
				self.bytes_outgoing = None
		else:
			self.bytes_outgoing = 0

	def parse_connection_status(self, field):
		field = field.upper()
		if field in ('X', '-', '+'):
			self.connection_status = field
		else:
			self.connection_status = None

	def parse_forwarded_for(self, field):
		if field != '-':
			self.forwarded_for = field
		else:
			self.forwarded_for = None

	def parse_method(self, field):
		field = field.upper()
		if field in ('CONNECT', 'DELETE', 'GET', 'HEAD', 'OPTIONS', 'POST', 'PUT', 'TRACE'):
			self.method = field
		else:
			self.method = None

	def parse_nexopia_userage(self, field):
		if field != '-':
			try:
				self.nexopia_userage = int(field)
			except ValueError:
				self.nexopia_userage = None
		else:
			self.nexopia_userage = None

	def parse_nexopia_userid(self, field):
		if field != '-':
			try:
				self.nexopia_userid = int(field)
			except ValueError:
				self.nexopia_userid = None
		else:
			self.nexopia_userid = None

	def parse_nexopia_userlocation(self, field):
		if field != '-':
			try:
				self.nexopia_userlocation = int(field)
			except ValueError:
				self.nexopia_userlocation = None
		else:
			self.nexopia_userlocation = None

	def parse_nexopia_usersex(self, field):
		field = field.lower()
		if field in ('female', 'male'):
			self.nexopia_usersex = field
		else:
			self.nexopia_usersex = None

	def parse_nexopia_usertype(self, field):
		field = field.lower()
		if field in ('anon', 'plus', 'user'):
			self.nexopia_usertype = field
		else:
			self.nexopia_usertype = None

	def parse_query_string(self, field):
		if field != '-':
			self.query_string = field
		else:
			self.query_string = None

	def parse_referer(self, field):
		if field != '-':
			self.referer = field
		else:
			self.referer = None

	def parse_remote_address(self, field):
		if field != '-':
			self.remote_address = field
		else:
			self.remote_address = None

	def parse_request_url(self, field):
		if field != '-':
			self.request_url = field
		else:
			self.request_url = None

	def parse_serial(self, field):
		try:
			self.serial = int(field)
		except ValueError:
			self.serial = None

	def parse_source(self, field):
		# field[1:-1] is substantially faster than field.strip('[]'), which is why we do it.
		self.source_address, colon, self.source_port = field[1:-1].partition(':')
		try:
			self.source_port = int(self.source_port)
		except ValueError:
			self.source_port = None

	def parse_status(self, field):
		try:
			self.status = int(field)
		except ValueError:
			self.status = None

	def parse_tag(self, field):
		self.tag = field

	def parse_time_used(self, field):
		try:
			self.time_used = int(field)
		except ValueError:
			self.time_used = None

	def parse_datetime(self, field, cache={}):
		# We cache the date/times in the cache dictionary after we parse each one.
		# This speeds our parsing up a very large amount; as dictionary lookups
		# are very fast and calls to both time.strptime() and time.mktime()
		# are very slow.
		if field in cache:
			self.date_time, self.unix_timestamp = cache[field]
		else:
			try:
				self.date_time = time.strptime(field, '[%Y-%m-%d %H:%M:%S]')
				self.unix_timestamp = time.mktime(self.date_time)
				cache[field] = (self.date_time, self.unix_timestamp)
			except (OverflowError, ValueError):
				self.date_time = None
				self.unix_timestamp = None

	def parse_user_agent(self, field):
		if field != '-':
			self.user_agent = field
		else:
			self.user_agent = None

class LogLine_v2(LogLine_v1):
	def parse(self, fields):
		self.parse_datetime(fields[0])
		self.parse_source(fields[1])
		self.parse_serial(fields[2])
		self.parse_tag(fields[3])
		self.parse_version(fields[4])
		self.parse_method(fields[5])
		self.parse_status(fields[6])
		self.parse_body_size(fields[7])
		self.parse_bytes_incoming(fields[8])
		self.parse_bytes_outgoing(fields[9])
		self.parse_time_used(fields[10])
		self.parse_connection_status(fields[11])
		self.parse_request_url(fields[12])
		self.parse_query_string(fields[13])
		self.parse_remote_address(fields[14])
		self.parse_host(fields[15])
		self.parse_user_agent(fields[16])
		self.parse_forwarded_for(fields[17])
		self.parse_referer(fields[18])
		self.parse_content_type(fields[19])
		self.parse_nexopia_userid(fields[20])
		self.parse_nexopia_userage(fields[21])
		self.parse_nexopia_usersex(fields[22])
		self.parse_nexopia_userlocation(fields[23])
		self.parse_nexopia_usertype(fields[24])

	def parse_content_type(self, field):
		if field != '-':
			self.content_type = field
		else:
			self.content_type = None

	def parse_host(self, field):
		if field != '-':
			self.host = field
		else:
			self.host = None

	def parse_version(self, field):
		# Using field[1:] is substantially faster than using
		# field.strip('v'), which is why we do it.
		try:
			self.version = int(field[1:])
		except ValueError:
			self.version = None

class LogLine(LogLine_v2):
	def parse(self, raw):
		self.raw = raw
		fields = self.raw.split('\x1e')
		try:
			if fields[4] == 'v2':
				LogLine_v2.parse(self, fields)
			else:
				LogLine_v1.parse(self, fields)
		except Exception, e:
			sys.stderr.write('self.raw: ' + self.raw + '\n')
			sys.stderr.write('fields: ' + str(fields) + '\n')
			raise e
