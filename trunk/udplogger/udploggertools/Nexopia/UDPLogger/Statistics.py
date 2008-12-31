#!/usr/bin/python
class UDPLoggerStatistic:
	def __init__(self):
		self.results = {}

	def __str__(self):
		s = '%s\n' % str(self.__class__)
		for unix_timestamp in self.results:
			s += '\t%d\n' % unix_timestamp
			for i in self.results[unix_timestamp]:
				s += '\t\t%-25s\t%s\n' % (str(i), str(self.results[unix_timestamp][i]))
		return s

class ContentTypeStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS content_type_statistics (
									timestamp INTEGER,
									content_type TEXT,
									count INTEGER,
									transferred INTEGER,
									PRIMARY KEY (timestamp, content_type)
								);''')
		for unix_timestamp in self.results:
			for content_type in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO content_type_statistics (
											timestamp,
											content_type,
											count,
											transferred
										) VALUES (
											?,
											?,
											0,
											0
										);''', (unix_timestamp, content_type))
				cursor.execute('''UPDATE content_type_statistics
										SET
											count = count + ?,
											transferred = transferred + ?
										WHERE
											timestamp = ? AND
											content_type = ?;''', (self.results[unix_timestamp][content_type]['count'], self.results[unix_timestamp][content_type]['transferred'], unix_timestamp, content_type))
		database_connection.commit()

	def update(self, log):
		if log.content_type is None:
			content_type = ""
		else:
			i = log.content_type.find(';')
			if i > -1:
				content_type = log.content_type[:i]
			else:
				content_type = log.content_type
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not content_type in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][content_type] = {}
			self.results[log.unix_timestamp][content_type]['count'] = 0
			self.results[log.unix_timestamp][content_type]['transferred'] = 0
		self.results[log.unix_timestamp][content_type]['count'] += 1
		self.results[log.unix_timestamp][content_type]['transferred'] += log.bytes_outgoing

class HitStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS hit_statistics (
									timestamp INTEGER,
									hits INTEGER,
									bytes_incoming INTEGER,
									bytes_outgoing INTEGER,
									PRIMARY KEY (timestamp)
								);''')
		for unix_timestamp in self.results:
			cursor.execute('''INSERT OR IGNORE INTO hit_statistics (
										timestamp,
										hits,
										bytes_incoming,
										bytes_outgoing
									) VALUES (
										?,
										0,
										0,
										0
									);''', (unix_timestamp,))
			cursor.execute('''UPDATE hit_statistics
									SET
										hits = hits + ?,
										bytes_incoming = bytes_incoming + ?,
										bytes_outgoing = bytes_outgoing + ?
									WHERE
										timestamp = ?;''', (self.results[unix_timestamp]['hits'], self.results[unix_timestamp]['bytes_incoming'], self.results[unix_timestamp]['bytes_outgoing'], unix_timestamp))
		database_connection.commit()

	def update(self, log):
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
			self.results[log.unix_timestamp]['bytes_incoming'] = 0
			self.results[log.unix_timestamp]['bytes_outgoing'] = 0
			self.results[log.unix_timestamp]['hits'] = 0
		if not log.bytes_incoming is None:
			self.results[log.unix_timestamp]['bytes_incoming'] += log.bytes_incoming
		if not log.bytes_outgoing is None:
			self.results[log.unix_timestamp]['bytes_outgoing'] += log.bytes_outgoing
		self.results[log.unix_timestamp]['hits'] += 1

class HostStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS host_statistics (
									timestamp INTEGER,
									host TEXT,
									count INTEGER,
									PRIMARY KEY (timestamp, host)
								);''')
		for unix_timestamp in self.results:
			for host in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO host_statistics (
											timestamp,
											host,
											count
										) VALUES (
											?,
											?,
											0
										);''', (unix_timestamp, host))
				cursor.execute('''UPDATE host_statistics
										SET
											count = count + ?
										WHERE
											timestamp = ? AND
											host = ?;''', (self.results[unix_timestamp][host], unix_timestamp, host))
		database_connection.commit()

	def update(self, log):
		if log.host is None:
			host = ""
		else:
			i = log.host.find(':')
			if i > -1:
				host = log.host[:i]
			else:
				host = log.host
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not host in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][host] = 0
		self.results[log.unix_timestamp][host] += 1

class StatusStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS status_statistics (
									timestamp INTEGER,
									status INTEGER,
									count INTEGER,
									PRIMARY KEY (timestamp, status)
								);''')
		for unix_timestamp in self.results:
			for status in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO status_statistics (
											timestamp,
											status,
											count
										) VALUES (
											?,
											?,
											0
										);''', (unix_timestamp, status))
				cursor.execute('''UPDATE status_statistics
										SET
											count = count + ?
										WHERE
											timestamp = ? AND
											status = ?;''', (self.results[unix_timestamp][status], unix_timestamp, status))
		database_connection.commit()

	def update(self, log):
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not log.status in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][log.status] = 0
		self.results[log.unix_timestamp][log.status] += 1

class TimeUsedStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS time_used_statistics (
									timestamp INTEGER,
									time_used INTEGER,
									count INTEGER,
									PRIMARY KEY (timestamp, time_used)
								);''')
		for unix_timestamp in self.results:
			for time_used in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO time_used_statistics (
											timestamp,
											time_used,
											count
										) VALUES (
											?,
											?,
											0
										);''', (unix_timestamp, time_used))
				cursor.execute('''UPDATE time_used_statistics
										SET
											count = count + ?
										WHERE
											timestamp = ? AND
											time_used = ?;''', (self.results[unix_timestamp][time_used], unix_timestamp, time_used))
		database_connection.commit()

	def update(self, log):
		if log.time_used > 10:
			time_used = -1
		else:
			time_used = log.time_used
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not time_used in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][time_used] = 0
		self.results[log.unix_timestamp][time_used] += 1

class UserSexStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS usersex_statistics (
									timestamp INTEGER,
									sex TEXT,
									count INTEGER,
									PRIMARY KEY (timestamp, sex)
								);''')
		for unix_timestamp in self.results:
			for usersex in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO usersex_statistics (
											timestamp,
											sex,
											count
										) VALUES (
											?,
											?,
											0
										);''', (unix_timestamp, usersex))
				cursor.execute('''UPDATE usersex_statistics
										SET
											count = count + ?
										WHERE
											timestamp = ? AND
											sex = ?;''', (self.results[unix_timestamp][usersex], unix_timestamp, usersex))
		database_connection.commit()

	def update(self, log):
		if log.nexopia_usersex == None:
			usersex = "unknown"
		else:
			usersex = log.nexopia_usersex
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not usersex in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][usersex] = 0
		self.results[log.unix_timestamp][usersex] += 1

class UserTypeStatistic(UDPLoggerStatistic):
	def save(self, database_connection):
		cursor = database_connection.cursor()
		cursor.execute('''CREATE TABLE IF NOT EXISTS usertype_statistics (
									timestamp INTEGER,
									type TEXT,
									count INTEGER,
									PRIMARY KEY (timestamp, type)
								);''')
		for unix_timestamp in self.results:
			for usertype in self.results[unix_timestamp]:
				cursor.execute('''INSERT OR IGNORE INTO usertype_statistics (
											timestamp,
											type,
											count
										) VALUES (
											?,
											?,
											0
										);''', (unix_timestamp, usertype))
				cursor.execute('''UPDATE usertype_statistics
										SET
											count = count + ?
										WHERE
											timestamp = ? AND
											type = ?;''', (self.results[unix_timestamp][usertype], unix_timestamp, usertype))
		database_connection.commit()

	def update(self, log):
		if log.nexopia_usertype == None:
			usertype = "unknown"
		else:
			usertype = log.nexopia_usertype
		if not log.unix_timestamp in self.results:
			self.results[log.unix_timestamp] = {}
		if not usertype in self.results[log.unix_timestamp]:
			self.results[log.unix_timestamp][usertype] = 0
		self.results[log.unix_timestamp][usertype] += 1
