#!/usr/bin/python

import inspect
import locale
locale.setlocale(locale.LC_NUMERIC, 'en_CA.UTF-8')
import os
import sqlite3
import sys
import time
import StringIO

#
# Set the HOME environment variable to a directory that the webserver can write
# to.  matplotlib needs to be able to write to this directory, annoyingly; and
# will explode if it cannot or if it otherwise encounters any errors with it.
#
os.environ['HOME'] = '/tmp/'
import matplotlib

#
# Configure matplotlib to use a non-GUI backend
#
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def available_graphs():
	module = sys.modules[__name__]
	results = []
	for object in module.__dict__:
		object = getattr(module, object)
		if inspect.isclass(object):
			if hasattr(object, 'csv') and hasattr(object, 'load') and hasattr(object, 'png') and hasattr(object, 'description'):
				results.append(object())
	results.sort(key=lambda x: x.description())
	return results

class UDPLoggerGraph:
	def __init__(self):
		self.plots = {}
		self.timestamps = []

	def add_datapoint(self, plot, timestamp, value):
		# Check if the plot given already exists and create it if it does not.
		# If we create the plot, set it to zero for all of the timestamps
		# that we have already passed.
		if plot not in self.plots:
			self.plots[plot] = list(0 for i in range(len(self.timestamps)))
		# Check if the timestamp we are dealing with is a new one, and if it is
		# append it to our list of timestamps and initialize all plots to have
		# zero for their value for the new timestamp.
		if len(self.timestamps) == 0 or self.timestamps[-1] != timestamp:
			self.timestamps.append(timestamp)
			for i in self.plots:
				self.plots[i].append(0)
		self.plots[plot][-1] = value

	def connect(self, database_path):
		self.db = sqlite3.connect(database_path)
		self.db.row_factory = sqlite3.Row
		self.db.text_factory = str

	def csv(self, start_timestamp, end_timestamp):
		self.load(start_timestamp, end_timestamp)
		s = ''
		s += '"Timestamp",'
		for plot in self.plots:
			s += '"' + self.series_label(plot) + '",'
		s = s.rstrip(',') + '\n'
		for i in range(len(self.timestamps)):
			s += '"' + str(self.timestamps[i]) + '",'
			for plot in self.plots:
				s += '"' + str(self.plots[plot][i]) + '",'
			s = s.rstrip(',') + '\n'
		return s

	def png(self, start_timestamp, end_timestamp):
		self.load(start_timestamp, end_timestamp)
		plt.figure(figsize=(10, 7.52))
		plt.rc('axes', labelsize=12, titlesize=14)
		plt.rc('font', size=10)
		plt.rc('legend', fontsize=7)
		plt.rc('xtick', labelsize=8)
		plt.rc('ytick', labelsize=8)
		plt.axes([0.08, 0.08, 1 - 0.27, 1 - 0.15])
		for plot in self.plots:
			plt.plot(self.timestamps, self.plots[plot], self.series_fmt(plot), label=self.series_label(plot))
		plt.axis('tight')
		plt.gca().xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda x, pos = None: time.strftime('%H:%M\n%b %d', time.localtime(x))))
		plt.gca().yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda x, pos = None: locale.format('%.*f', (0, x), True)))
		plt.grid(True)
		plt.legend(loc=(1.003, 0))
		plt.xlabel('Time/Date')
		plt.title(self.description() + '\n%s to %s' % (time.strftime('%H:%M %d-%b-%Y', time.localtime(start_timestamp)), time.strftime('%H:%M %d-%b-%Y', time.localtime(end_timestamp))))
		output_buffer = StringIO.StringIO()
		plt.savefig( output_buffer, format='png' )
		return output_buffer.getvalue()


	def series_fmt(self, series, fmt_idx=[-1]):
		'''Returns the matplotlib format string that should be used to graph the
		given series.  For a specification of the matplotlib format string, see.
		[http://matplotlib.sourceforge.net/api/pyplot_api.html#matplotlib.pyplot.plot]'''
		formats = ['b-', 'g-', 'r-', 'c-', 'm-', 'y-', 'k-']
		fmt_idx[0] += 1
		if fmt_idx[0] >= len(formats):
			fmt_idx[0] = 0
		return formats[fmt_idx[0]]

	def series_label(self, series):
		'''Returns the user-facing label (must be a string) that should be used
		to refer to the given series.'''
		if series == '':
			return '(None)'
		return str(series)

class ContentTypeHitsGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT timestamp, content_type, count FROM content_type_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['content_type'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'Hits per Content Type'

class ContentTypeTransferredGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT timestamp, content_type, transferred FROM content_type_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['content_type'], row['timestamp'], row['transferred'] / float(1024 * 1024))
		cursor.close()

	def description(self):
		return 'Transferred per Content Type (MByte)'

class HostGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT * FROM host_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['host'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'Hits per Virtual Host'

class StatusGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT * FROM status_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['status'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'Status Codes'

class TotalHitsGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT timestamp, hits FROM hit_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint('Hits', row['timestamp'], row['hits'])
		cursor.close()

	def description(self):
		return 'Hits (Total)'

class TotalTransferredGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT timestamp, bytes_incoming, bytes_outgoing FROM hit_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint('Incoming', row['timestamp'], row['bytes_incoming'] / float(1024 * 1024))
			self.add_datapoint('Outgoing', row['timestamp'], row['bytes_outgoing'] / float(1024 * 1024))
		cursor.close()

	def description(self):
		return 'Transferred (MByte) (Total)'

class TimeUsedGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT * FROM time_used_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['time_used'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'Request Completion Times'

	def series_label(self, series):
		if series == -1:
			return '11+ seconds'
		elif series in range(11):
			return str(series) + '-' + str(series + 1) + ' seconds'
		return UDPLoggerGraph.series_label(self, series)

class UserSexGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT * FROM usersex_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['sex'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'User Sex'

class UserTypeGraph(UDPLoggerGraph):
	def load(self, start_timestamp, end_timestamp):
		cursor = self.db.cursor()
		cursor.execute('SELECT * FROM usertype_statistics WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp', (start_timestamp, end_timestamp))
		for row in cursor:
			self.add_datapoint(row['type'], row['timestamp'], row['count'])
		cursor.close()

	def description(self):
		return 'User Type'

	def series_fmt(self, series):
		if series == 'unknown':
			return 'k:+'
		elif series == 'anon':
			return 'c:x'
		elif series == 'plus':
			return 'r-.*'
		elif series == 'user':
			return 'b-.d'
		return UDPLoggerGraph.series_fmt(self, series)

	def series_label(self, series):
		if series == 'unknown':
			return 'Unknown'
		elif series == 'anon':
			return 'Anonymous'
		elif series == 'plus':
			return 'Plus Users'
		elif series == 'user':
			return 'Users'
		return UDPLoggerGraph.series_label(self, series)
