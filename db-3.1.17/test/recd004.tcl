# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: recd004.tcl,v 11.14 2000/04/21 18:36:22 krinsky Exp $
#
# Recovery Test #4.
# Verify that we work correctly when big keys get elevated.
proc recd004 { method {select 0} } {
	source ./include.tcl
	global rand_init

	set opts [convert_args $method ""]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 } {
		puts "Recd004 skipping for method $method"
		return
	}
	puts "Recd004: $method big-key on internal page recovery tests"

	berkdb srand $rand_init

	cleanup $testdir
	set testfile recd004.db
	set testfile2 recd004-2.db
	set eflags "-create -txn -home $testdir"
	puts "\tRecd004.a: creating environment"
	set env_cmd "berkdb env $eflags"
	set dbenv [eval $env_cmd]
	error_check_bad dbenv $dbenv NULL

	# Create the databases. We will use a small page size so that we
	# elevate quickly
	set oflags "-create -mode 0644 \
	    $omethod -env $dbenv $opts -pagesize 512 $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	set oflags "-create -mode 0644 \
	    $omethod -env $dbenv $opts -pagesize 512 $testfile2"
	set db [eval {berkdb_open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	reset_env $dbenv

	# List of recovery tests: {CMD MSG} pairs
	set slist {
		{ {big_populate DB TXNID $n} "Recd004.b: big key elevation"}
		{ {unpopulate DB TXNID 0} "Recd004.c: Remove keys"}
	}

	# If pages are 512 bytes, then adding 512 key/data pairs
	# should be more than sufficient.
	set n 512
	foreach pair $slist {
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

	puts "\tRecd004.d: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec ./db_printlog -h $testdir > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}
