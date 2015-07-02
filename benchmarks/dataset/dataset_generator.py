#! usr/bin/python 

import argparse
import random
import string

parser = argparse.ArgumentParser()
parser.add_argument("-si", "--signed",
                    help="to generate a pseudo aleatory number of type <int>\n", 
                    action="store_true")
parser.add_argument("-ui", "--unsigned", 
                    help="to generate a pseudo aleatory number of type <unsigned int>\n", 
                    action="store_true")
parser.add_argument("-fl", "--floating", 
                    help="to generate a pseudo aleatory number of type <float>\n", 
                    action="store_true")
parser.add_argument("-st", "--string", 
                    help="to generate a pseudo aleatory string of type <string>\n", 
                    action="store_true")
parser.add_argument("-a", "--all",
                    help="to generate all pseudo aleatory datasets type avalaibles\n",
                    action="store_true")
args = parser.parse_args()

array=[10000, 100000, 500000, 1000000, 5000000, 10000000]

if args.signed:
  print "Signed generator has been activated."
  for i in range(0,6):
    buf="signed_intergers/"+str(array[i])+"_interger.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(-9223372036854775808,9223372036854775807)
      r=int(r)
      dataset.write('%d\n' % r)
    dataset.close()


if args.unsigned:
  print "Unsigned generator has been activated."
  for i in range(0,6):
    buf="unsigned_intergers/"+str(array[i])+"_unsigned.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(0,18446744073709551615)
      r=int(r)
      dataset.write('%d\n' % r)
    dataset.close()


if args.floating:
  print "floating generator has been activated."
  for i in range(0,6):
    buf="floating/"+str(array[i])+"_floating.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(1.17549e-38,3.40282e+38)
      r=str(r)
      dataset.write('%s\n' % r)
    dataset.close

if args.string:
  print "string generator has been activated."
  for i in range(0,6):
    buf="strings/"+str(array[i])+"_string.data"
    dataset=open(buf,'w')
    print "printing ", array[i] ," args"
    for j in range(0, array[i]):
      r=(''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for i in range(10)))
      dataset.write('%s\n' % r)
    dataset.close()

if args.all:
  #INTERGERS
  print "Signed generator has been activated."
  for i in range(0,6):
    buf="signed_intergers/"+str(array[i])+"_interger.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(-9223372036854775808,9223372036854775807)
      r=int(r)
      dataset.write('%d\n' % r)
    dataset.close()
  #UNSIGNED INTERGERS
  print "Unsigned generator has been activated."
  for i in range(0,6):
    buf="unsigned_intergers/"+str(array[i])+"_unsigned.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(0,18446744073709551615)
      r=int(r)
      dataset.write('%d\n' % r)
    dataset.close()
  #FLOATING
  print "floating generator has been activated."
  for i in range(0,6):
    buf="floating/"+str(array[i])+"_floating.data"
    dataset=open(buf,'w')
    print "printing ",  array[i]  ," args"
    for j in range(0,array[i]):
      r=random.uniform(1.17549e-38,3.40282e+38)
      r=str(r)
      dataset.write('%s\n' % r)
    dataset.close
  #STRINGS
  print "string generator has been activated."
  for i in range(0,6):
    buf="strings/"+str(array[i])+"_string.data"
    dataset=open(buf,'w')
    print "printing ", array[i] ," args"
    for j in range(0, array[i]):
      r=(''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for i in range(10)))
      dataset.write('%s\n' % r)
    dataset.close()