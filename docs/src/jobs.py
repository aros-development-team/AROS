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
# none   - print no jobs
#
# stats  - print some statistics on the end
# long - print stats per library
#
'''This module allows to read the jobs.dat DB. It will collect
the statistics about all jobs. You can access the following
properties in this module:

jobs - A dictionary of all libraries. The key is the long library
name.
__jobs - The same dictionary but with the abbreviated name of the
library as key.
'''

import os, sys, fileinput, string

from abbreviations import abbreviations

class Library:
    '''A summary of the status of all jobs in a library (for want of a
    better word). It has these properties:

    name - The abbreviated name
    longName - The full/long name (as by the abbreviations module)
    jobsDone, jobsInWork, jobsTodo - The number of jobs with the
	    respective status.
    self.jobsTotal - The total amount of jobs.
    jobs - A dictionary with the informations about the job.
	    Each job is tuple of id, status, login and comment.
    '''
    
    def __init__ (self, name):
	self.name = name
	self.longName = abbreviations[name]
	if not self.longName or self.name == self.longName:
	    print `self.name`, `self.longName`
	self.jobsDone, self.jobsInWork, self.jobsTodo = 0, 0, 0
	self.jobsTotal = 0
	self.status = [0.0, 0.0, 0.0]
	self.jobs = {}

    def addJob (self, id, status, login, comment):
	self.jobs[id] = (id, status, login, comment)
	self.jobsTotal = self.jobsTotal + 1

	if status == 0:
	    self.jobsTodo = self.jobsTodo + 1
	elif status == 1:
	    self.jobsInWork = self.jobsInWork + 1
	elif status == 2:
	    self.jobsDone = self.jobsDone + 1
	else:
	    raise 'Illegal status %s in %s%s' % (`status`, self.name, `id`)

    def update (self):
	if self.jobsTotal:
	    self.status[0] = 100.0*self.jobsTodo/self.jobsTotal
	    self.status[1] = 100.0*self.jobsInWork/self.jobsTotal
	    self.status[2] = 100.0*self.jobsDone/self.jobsTotal

    def printJob (self, job):
	# Has this job an ID ?
	if job[0] != '':
	    print '%s%05d;%d;%s;%s' % (self.name, job[0], job[1], job[2], job[3])
	else:
	    print '%s;%d;%s;%s' % (self.name, job[1], job[2], job[3])
	

    def __hash__ (self):
	return hash (self.longName)

    def __cmp__ (self, other):
	if not other:
	    return 1
	
	return cmp (self.longName, other.longName)

# Libraries with long
jobs = {}
# and their short names.
__jobs = {}

# Read in file
try:
    path = os.path.dirname (__file__)
except:
    path = os.curdir
filename = os.path.join (path, 'jobs.dat')

for line in fileinput.input (filename):
    jobid, status, login, comment = string.split (string.rstrip (line), ';')
    status = int (status)

    i, j = 0, len(jobid)
    while i < j and jobid[j-1] in string.digits:
	j = j - 1

    name = jobid[:j]
    #print name, jobid[j:]
    id = jobid[j:]
    if id:
	id = int (id)
    else:
	id = name
	name = 'other'

    lib = __jobs.get (name, None)
    if not lib:
	lib = Library (name)
	__jobs[name] = lib
	jobs[lib.longName] = lib

    lib.addJob (id, status, login, comment)
    
# Keeps track of statistics
jobsDone   = 0
jobsInWork = 0
jobsTodo   = 0

# Parse arguments
for lib in __jobs.values ():
    lib.update ()
    # 'other' jobs don't count
    if lib.name == 'other':
	continue

    jobsDone = jobsDone + lib.jobsDone
    jobsInWork = jobsInWork + lib.jobsInWork
    jobsTodo = jobsTodo + lib.jobsTodo

jobsTotal  = jobsDone + jobsInWork + jobsTodo
status = [
    100.0*jobsTodo/jobsTotal,
    100.0*jobsInWork/jobsTotal,
    100.0*jobsDone/jobsTotal,
]

# These are jobs which are not related to the AROS core.
if __jobs.has_key ('other'):
    otherJobs = __jobs['other']
else:
    otherJobs = Library ('other')
    otherJobs.update ()

#print jobsDone, jobsInWork, jobsTodo, jobsTotal

if __name__ == '__main__':
    showStats =  0   # Don't show stats as default
    longStatus = 0   # Don't show the verbose statistics
    listMode  =  0   # Print jobs that are todo as default

    for argument in sys.argv[1:]:
	if argument == 'stats':
	    showStats = 1
	elif argument == 'done':
	    listMode = 2
	elif argument == 'inwork':
	    listMode = 1
	elif argument == 'todo':
	    listMode = 0
	elif argument == 'none':
	    listMode = -1
	elif argument == 'long':
	    longStatus = 1

    list = jobs.keys ()
    list.sort ()
    for name in list:
	lib = jobs[name]

	ids = lib.jobs.keys ()
	ids.sort ()
	for id in ids:
	    job = lib.jobs[id]
	    if job[1] == listMode:
		lib.printJob (job)

    # Print statistics
    if showStats:
	if longStatus:
	    for name in list:
		lib = jobs[name]
		print '%-40s %6.2f%% %6.2f%% %6.2f%% = %6.2f%%' % (
		    lib.name, lib.status[0], lib.status[1], lib.status[2],
			lib.status[0] + lib.status[1] + lib.status[2])
		print '%40d %6d  %6d  %6d = %6d' % (
		    lib.jobsTotal, lib.jobsTodo, lib.jobsInWork, lib.jobsDone,
			lib.jobsTodo + lib.jobsInWork + lib.jobsDone)

	print
	print 'Statistics:'
	print 'Jobs todo   : %4d (%6.2f%%)' % (jobsTodo, status[0])
	print 'Jobs in work: %4d (%6.2f%%)' % (jobsInWork, status[1])
	print 'Jobs done   : %4d (%6.2f%%)' % (jobsDone, status[2])
	print
	print 'Jobs total  : %4d' % jobsTotal
	print
	print 'Other jobs todo   : %4d (%6.2f%%)' % (otherJobs.jobsTodo, otherJobs.status[0])
	print 'Other jobs in work: %4d (%6.2f%%)' % (otherJobs.jobsInWork, otherJobs.status[1])
	print 'Other jobs done   : %4d (%6.2f%%)' % (otherJobs.jobsDone, otherJobs.status[2])
	print

    if not jobsTotal:
	raise 'jobsTotal == 0'
