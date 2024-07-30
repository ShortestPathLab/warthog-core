#!/usr/bin/env python3

from pathlib import Path
import re
import sys
import collections as cl
import shutil

root = Path(__file__).resolve().parent.parent
root_inc = root/'include/warthog'
root_src = root/'src'

class SrcFile:
	def __init__(self, fname):
		self.fname:Path = Path(fname).resolve()
		self.relative:Path = None
		self.header:bool = False
		try:
			self.relative = self.fname.relative_to(root_inc)
			self.header = True
		except ValueError: # not in include directory
			self.relative = self.fname.relative_to(root_src)
		# will throw if not in include or src directory
		self.namespace:tuple[str,...] = self.relative.parts[0:-1]
		self.data:list[str]|None = None
		self.preamble_end = 0 # the end of any license, is 0 if none
		self.blank_file = False
		self.lines = 0
	
	def load(self):
		# loads file into data, trimming blank start/end lines
		with open(self.fname) as f:
			self.data = list(map(lambda g: g.removesuffix('\n'), f.readlines()))
		for start_line in range(len(self.data)):
			line = self.data[start_line].strip()
			if len(line) != 0 and not line[0] in ('*', '/'): # skip comment lines
				break
		self.preamble_end = start_line
		while len(self.data) > self.preamble_end and len(self.data[-1].strip()) == 0:
			del self.data[-1]
		self.lines = len(self.data) - self.preamble_end
		self.blank_file = self.lines == 0

class SrcAlter:
	def __init__(self, src:SrcFile):
		self.src = src
		self.issues:list[str] = []

	def line_conv(self, line:str, no:int) -> str|None:
		raise NotImplemented()
	
	def issue(self, no:int, message):
		print(f'line {no}: {message}', file=sys.stderr)

class GuardAlter(SrcAlter):
	def __init__(self, src:SrcFile):
		super().__init__(src)
		self.guard = '_'.join(map(str.upper, ['WARTHOG', *src.namespace, src.fname.name.replace('.','_')]))
		self.has_guard = src.header
		
	
	def line_conv(self, line:str, no:int) -> str|None:
		if not self.has_guard:
			return None
		if no == 0: # header gurad check
			if line.rstrip().startswith('#ifndef'):
				return f'#ifndef {self.guard}'
			else:
				self.has_guard = False
				self.issue(no, 'missing header guard')
		elif no == 1: # define header guard
			if line.rstrip().startswith('#define'):
				return f'#define {self.guard}'
			else:
				self.issue(no, 'header guard missing define')
		elif no == self.src.lines-1:
			if line.rstrip().startswith('#endif'):
				return f'#endif // {self.guard}'
			else:
				self.issue(no, 'header guard not at end of file')
		return None

class IncludeAlter(SrcAlter):
	INCLUDE = re.compile('^#include ["<]([^">]+)"$')
	FILES: dict[str,list[Path]]|None = None

	def line_conv(self, line:str, no:int) -> str|None:
		line = line.strip()
		res = re.fullmatch(IncludeAlter.INCLUDE, line)
		if res is None:
			return None
		if IncludeAlter.FILES is None:
			IncludeAlter.gen_files()
		fname = Path(res[1])
		includes = IncludeAlter.FILES.get(fname.name, None)
		if includes is None:
			self.issue(no, f'file {fname.name} not found in include')
			return None
		if len(includes) > 1:
			self.issue(no, f'file {fname.name} has multiple solutions')
			return None
		if self.src.header:
			# header checks if include should be a relative include
			try:
				relfile = includes[0].relative_to(self.src.relative.parent)
				return f'#include "{relfile}"'
			except ValueError:
				pass
			except Exception as e:
				self.issue(no, e)
		return f'#include <{includes[0]}>'
	
	@staticmethod
	def gen_files():
		lookup = cl.defaultdict(list)
		for f in root_inc.rglob('*.*'):
			lookup[f.name].append(f.relative_to(root_inc))
		IncludeAlter.FILES = lookup

def conv_file(file:str, alters: list):
	# alters is list of alter constructors
	file:Path = Path(file)
	src = SrcFile(file)
	src.load()
	run_alters: list[SrcAlter] = [a(src) for a in alters]
	shutil.copyfile(file, file.with_name(f'{file.name}.backup'))
	with open(file, 'w', buffering=4096, encoding='utf-8', newline='\n') as output:
		def writeln(line:str):
			output.write(line + '\n')
		end = src.preamble_end
		for no in range(0, end):
			writeln(src.data[no])
		for no in range(src.lines):
			line = src.data[no+end]
			for a in run_alters:
				new_line = a.line_conv(line, no)
				if new_line is not None:
					print(f'conv: {line!r} => {new_line!r}')
					line = new_line
					break
			writeln(line)

def _main():
	conv_file(sys.argv[1], [GuardAlter, IncludeAlter])

if __name__ == '__main__':
	_main()
