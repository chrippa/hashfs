# vim: set fileencoding=utf-8 filetype=python :

from logging import fatal, info
import os

srcdir = '.'
blddir = '_build_'
bckenddir = 'src/backends'

def is_backend(x):
	return os.path.exists(os.path.join(bckenddir, x, 'wscript'))

hashfs = ['src/hashfs']
libs = ['src/lib/libanidb', 'src/lib/libanidb/tests']
backends = [os.path.join(bckenddir, p) for p in os.listdir(bckenddir) if is_backend(p)]

subdirs = hashfs + libs + backends


def set_options(opt):
	opt.add_option('--debug', action = 'store_true', default = True,
	               help = 'Enable debug')

	for dir in subdirs:
		opt.sub_options(dir)

def configure(conf):
	import Options

	conf.check_tool('gcc')

	for dir in subdirs:
		conf.sub_config(dir)

def build(bld):
	bld.add_subdirs(subdirs)
