import sys
import os
import dircache
import string

proto_dir = 'make_example_prototype/'
target_dir = '../src/examples/'

def make_example(projname):
  os.mkdir(target_dir + projname)
  for fname in dircache.listdir(proto_dir):
    dest_name = string.replace(fname, 'prototype', projname)
    out = open(target_dir + projname + '/' + dest_name, "wb")
    for line in open(proto_dir + fname):
      line = string.replace(line, 'prototype', projname)
      out.write(line)


num_args = len(sys.argv)

if num_args == 1 or "--help" in sys.argv:
  print("\nmakes a new octet project in src/examples")
  print("\nusage: make_example.py projectname")
  exit()

for i in range(1,num_args):
  arg = sys.argv[i];
  if arg[0] != '-':
    make_example(arg)
  else:
    print("unrecognised option " + arg)
    exit()
