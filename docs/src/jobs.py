#!/bin/env python
#
# NAME
#
# jobs.py
#
# SYNOPSIS
#
# jobs.py [done|inwork|todo] [stats]
#
# FUNCTION
#
# Prints all jobs that are done, in work or todo - depending on the arguments.
# If 'stats' is given, some statistics will be printed on the end.
#
# If no arguments are given, the default is to print jobs todo without stats.
#
# ARGUMENTS
#
# done   - print all jobs that are done
# inwork - print all jobs that are in work
# todo   - print all jobs that are still todo
#
# stats  - print some statistics on the end
#
# TODO
#
# Make it more useful.

import os, sys

# Read in file

file = open( 'jobs.dat', 'r' )

lines = file.readlines()

file.close()

# Keeps track of statistics

jobsDone   = 0
jobsInWork = 0
jobsTodo   = 0

jobsTotal  = 0

# Parse arguments

showStats =  0   # Don't show stats as default
listMode  = '0'  # Print jobs that are todo as default

for argument in sys.argv[1:]:
    if argument == 'stats':
	showStats = 1

    elif argument == 'done':
	listMode = '2'
    elif argument == 'inwork':
	listMode = '1'
    elif argument == 'todo':
	listMode = '0'

# Parse the file

for line in lines:
    line = line.strip()
    fields = line.split( ';' )

    if fields[1] == listMode:
	print line

    if fields[1] == '0':
	jobsTodo = jobsTodo + 1

    elif fields[1] == '1':
	jobsInWork = jobsInWork + 1

    elif fields[1] == '2':
	jobsDone = jobsDone + 1

jobsTotal = jobsDone + jobsInWork + jobsTodo

# Print statistics

if showStats:
    print ''
    print 'Statistics:'
    print 'Jobs done   :', jobsDone,   '(', 100 * jobsDone / jobsTotal, '% )'
    print 'Jobs in work:', jobsInWork, '(', 100 * jobsInWork / jobsTotal, '% )'
    print 'Jobs todo   :', jobsTodo,   '(', 100 * jobsTodo / jobsTotal, '% )'
    print ''
    print 'Jobs total  :', jobsTotal

