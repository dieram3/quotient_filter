#! usr/bin/python 

# 10K , 100K, 500K, 1M, 5M, 10M

import argparse
import random

parser = argparse.ArgumentParser()
parser.add_argument("-si", "--signed",
                    help="to generate a pseudo aleatory number of type <int>\n", 
                    action="store_true")
parser.add_argument("-ui", "--unsigned", 
                    help="to generate a pseudo aleatory number of type <uint>\n", 
                    action="store_true")
parser.add_argument("-sf", "--floating", 
                    help="to generate a pseudo aleatory number of type <float>\n", 
                    action="store_true")
parser.add_argument("-uf", "--ufloating", 
                    help="to generate a pseudo aleatory number of type <ufloat>\n", 
                    action="store_true")
parser.add_argument("-st", "--string", 
                    help="to generate a pseudo aleatory string of type <string>\n", 
                    action="store_true")
args = parser.parse_args()

array=[10000, 100000, 500000, 1000000, 5000000, 10000000]

if args.signed:
  print "Signed generator has been activated."
  for i in range(0,6):
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(-9223372036854775808,9223372036854775807)
      r=int(r)
      print j,".- ",r
    raw_input()


if args.unsigned:
  print "Unsigned generator has been activated."
  for i in range(0,6):
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(0,18446744073709551615)
      r=int(r)
      print j,".- ",r
    raw_input()


if args.floating:
  print "Unsigned generator has been activated."
  for i in range(0,6):
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(1.17549e-38,3.40282e+38)
      r=int(r)
      print j,".- ",r
    raw_input()
