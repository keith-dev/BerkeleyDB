# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2005
#	Sleepycat Software.  All rights reserved.
#
# $Id: dead007.tcl,v 12.1 2005/06/16 20:23:52 bostic Exp $
#
# TEST	dead007
# TEST	Tests for locker and txn id wraparound.
proc dead007 { {tnum "007"} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set save_curid $lock_curid
	set save_maxid $lock_maxid
	puts "Dead$tnum.a -- wrap around"
	set lock_curid [expr $lock_maxid - 2]
	dead001 "2 10" "ring clump" "0" $tnum
	## Oldest/youngest breaks when the id wraps
	# dead003 "4 10"
	dead004 $tnum

	puts "Dead$tnum.b -- extend space"
	set lock_maxid [expr $lock_maxid - 3]
	set lock_curid [expr $lock_maxid - 1]
	dead001 "4 10" "ring clump" "0" $tnum
	## Oldest/youngest breaks when the id wraps
	# dead003 "10"
	dead004 $tnum

	set lock_curid $save_curid
	set lock_maxid $save_maxid
	# Return the empty string so we don't return lock_maxid.
	return ""
}
