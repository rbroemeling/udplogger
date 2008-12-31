#!/usr/bin/python

import inspect
import sys

def available_graphs():
	module = sys.modules[__name__]
	results = []
	for object in module.__dict__:
		object = getattr(module, object)
		if inspect.isclass(object):
			if hasattr(object, 'csv') and hasattr(object, 'png') and hasattr(object, 'description'):
				results.append(object())
	results.sort(key=lambda x: x.description())
	return results

class UDPLoggerGraph:
	def series_fmt(self, series, fmt_idx=[-1]):
		formats = ['b-', 'g-', 'r-', 'c-', 'm-', 'y-', 'k-']
		fmt_idx[0] += 1
		if fmt_idx[0] >= len(formats):
			fmt_idx[0] = 0
		return formats[fmt_idx[0]]

	def series_label(self, series):
		return series

class UserSexGraph:
	def description(self):
		return "User Sex"

class UserTypeGraph:
	def description(self):
		return "User Type"
