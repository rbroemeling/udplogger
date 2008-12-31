#!/usr/bin/python

class UDPLoggerGraph:
	def series_fmt(self, series, fmt_idx=[-1]):
		formats = ['b-', 'g-', 'r-', 'c-', 'm-', 'y-', 'k-']
		fmt_idx[0] += 1
		if fmt_idx[0] >= len(formats):
			fmt_idx[0] = 0
		return formats[fmt_idx[0]]

	def series_label(self, series):
		return series
