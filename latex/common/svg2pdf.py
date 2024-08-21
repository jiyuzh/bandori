#!/usr/bin/env python3

import os
import sys
import concurrent.futures
from subprocess import Popen
from pathlib import Path

def waitForResponse(x): 
	out, err = x.communicate() 
	if x.returncode < 0: 
		r = "Popen returncode: " + str(x.returncode) 
		raise OSError(r)

def convertSvgFile(cmd, svg):
	# Extension check
	path = Path(svg)
	if not path.suffix.upper() == '.SVG':
		return False

	# Modification check
	pdf = path.with_suffix('.pdf')
	if Path(pdf).exists() and os.path.getmtime(pdf) > os.path.getmtime(svg):
		print(f'Skipped: {svg}')
		return False

	# Real job
	x = Popen([cmd, svg, '--export-filename=%s' % pdf])
	print(f'Rendering: {svg} -> {pdf}')
	try:
		waitForResponse(x)
		print(f'Rendered: {svg} -> {pdf}')
		return True
	except:
		return False

def convertSvgFileMappable(args):
	cmd, svg = args
	convertSvgFile(cmd, svg)

def convertSvgFiles(cmd, files):
	args = ((cmd, file) for file in files)

	with concurrent.futures.ProcessPoolExecutor() as pool:
		for ret in pool.map(convertSvgFileMappable, args):
			pass

if __name__ == "__main__":
	cmd = sys.argv[1]

	if len(sys.argv) > 2:
		files = sys.argv[2:]
	else:
		files = list(Path('.').rglob('*.svg'))

	convertSvgFiles(cmd, files)
