# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999
#	Sleepycat Software.  All rights reserved.
#
#	@(#)recd003.tcl	11.8 (Sleepycat) 11/10/99
#
# Recovery Test 3.
# Test all the duplicate log messages and recovery operations.  We make
# sure that we exercise all possible recovery actions: redo, undo, undo
# but no fix necessary and redo but no fix necessary.
proc recd003 { method {select 0} } {
	source ./include.tcl

	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 } {
		puts "Recd003 skipping for method $method"
		return
	}
	puts "Recd003: $method duplicate recovery tests"

	cleanup $testdir
	# See comment in recd001.tcl for why there are two database files...
	set testfile recd003.db
	set testfile2 recd003-2.db
	set eflags "-create -log -lock -mpool -txn -home $testdir"

	puts "\tRecd003.a: creating environment"
	set env_cmd "berkdb env $eflags"
	set dbenv [eval $env_cmd]
	error_check_bad dbenv $dbenv NULL

	# Create the databases.
	set oflags "-create -mode 0644 $omethod -dup -env $dbenv $testfile"
	set db [eval {berkdb open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	set oflags "-create -mode 0644 $omethod -dup -env $dbenv $testfile2"
	set db [eval {berkdb open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	reset_env $dbenv

	# These are all the data values that we're going to need to read
	# through the operation table and run the recovery tests.
	set n 10
	set dupn 2000
	set bign 500

	# List of recovery tests: {CMD MSG} pairs
	set dlist {
	{ {populate DB $omethod TXNID $n 1 0}
	    "Recd003.b: add dups"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.c: remove dups all at once"}
	{ {populate DB $omethod TXNID $n 1 0}
	    "Recd003.d: add dups (change state)"}
	{ {unpopulate DB TXNID 0}
	    "Recd003.e: remove dups 1 at a time"}
	{ {populate DB $omethod TXNID $dupn 1 0}
	    "Recd003.f: dup split"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.g: remove dups (change state)"}
	{ {populate DB $omethod TXNID $n 1 1}
	    "Recd003.h: add big dup"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.i: remove big dup all at once"}
	{ {populate DB $omethod TXNID $n 1 1}
	    "Recd003.j: add big dup (change state)"}
	{ {unpopulate DB TXNID 0}
	    "Recd003.k: remove big dup 1 at a time"}
	{ {populate DB $omethod TXNID $bign 1 1}
	    "Recd003.l: split big dup"}
	}

	foreach pair $dlist {
		set cmd [subst [lindex $pair 0]]
		set msg [lindex $pair 1]
		if { $select != 0 } {
			set tag [lindex $msg 0]
			set tail [expr [string length $tag] - 2]
			set tag [string range $tag $tail $tail]
			if { [lsearch $select $tag] == -1 } {
				continue
			}
		}
		op_recover abort $testdir $env_cmd $testfile $cmd $msg
		op_recover commit $testdir $env_cmd $testfile $cmd $msg
		op_recover prepare $testdir $env_cmd $testfile2 $cmd $msg
		op_recover prepare-abort $testdir $env_cmd $testfile2 \
			$cmd $msg
		op_recover prepare-commit $testdir $env_cmd $testfile2 \
			$cmd $msg
	}

	puts "\tRecd003.m: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec ./db_printlog -h $testdir > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	exec $RM $tmpfile
}
