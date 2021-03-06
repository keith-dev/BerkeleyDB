# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2001
#	Sleepycat Software.  All rights reserved.
#
# $Id: test092.tcl,v 11.8 2001/08/03 16:39:48 bostic Exp $
#
# TEST	test092
# TEST	Test of DB_DIRTY_READ [#3395]
# TEST
# TEST	We set up a database with nentries in it.  We then open the
# TEST	database read-only twice.  One with dirty read and one without.
# TEST	We open the database for writing and update some entries in it.
# TEST	Then read those new entries via db->get (clean and dirty), and
# TEST	via cursors (clean and dirty).
proc test092 { method {nentries 1000} args } {
	source ./include.tcl
	#
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test092 skipping for env $env"
		return
	}
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test092: Dirty Read Test $method $nentries"

	# Create the database and open the dictionary
	set testfile test092.db
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	env_cleanup $testdir

	set lmax [expr $nentries * 2]
	set lomax [expr $nentries * 2]
	set env [berkdb env -create -txn -home $testdir \
	    -lock_max_locks $lmax -lock_max_objects $lomax]
	error_check_good dbenv [is_valid_env $env] TRUE

	set db [eval {berkdb_open -env $env -create \
	    -mode 0644 $omethod} $args {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Here is the loop where we put each key/data pair.
	# Key is entry, data is entry also.
	puts "\tTest092.a: put loop"
	set count 0
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
		}
		set ret [eval {$db put} {$key [chop_data $method $str]}]
		error_check_good put:$db $ret 0
		incr count
	}
	close $did
	error_check_good close:$db [$db close] 0

	puts "\tTest092.b: Opening all the handles"
	#
	# Open all of our handles.
	# We need:
	# 1.  Our main txn (t).
	# 2.  A txn that can read dirty data (tdr).
	# 3.  A db handle for writing via txn (dbtxn).
	# 4.  A db handle for clean data (dbcl).
	# 5.  A db handle for dirty data (dbdr).
	# 6.  A cursor handle for dirty txn data (clean db handle using
	#    the dirty txn handle on the cursor call) (dbccl1).
	# 7.  A cursor handle for dirty data (dirty on get call) (dbcdr0).
	# 8.  A cursor handle for dirty data (dirty on cursor call) (dbcdr1).
	set t [$env txn]
	error_check_good txnbegin [is_valid_txn $t $env] TRUE

	set tdr [$env txn -dirty]
	error_check_good txnbegin:dr [is_valid_txn $tdr $env] TRUE

	set dbtxn [eval {berkdb_open -env $env -dirty \
	    -mode 0644 $omethod} {$testfile}]
	error_check_good dbopen:dbtxn [is_valid_db $dbtxn] TRUE

	set dbcl [eval {berkdb_open -env $env \
	    -rdonly -mode 0644 $omethod} {$testfile}]
	error_check_good dbopen:dbcl [is_valid_db $dbcl] TRUE

	set dbdr [eval {berkdb_open -env $env -dirty \
	    -rdonly -mode 0644 $omethod} {$testfile}]
	error_check_good dbopen:dbdr [is_valid_db $dbdr] TRUE

	set dbccl [$dbcl cursor -txn $tdr]
	error_check_good dbcurs:dbcl [is_valid_cursor $dbccl $dbcl] TRUE

	set dbcdr0 [$dbdr cursor]
	error_check_good dbcurs:dbdr0 [is_valid_cursor $dbcdr0 $dbdr] TRUE

	set dbcdr1 [$dbdr cursor -dirty]
	error_check_good dbcurs:dbdr1 [is_valid_cursor $dbcdr1 $dbdr] TRUE

	#
	# Now that we have all of our handles, change all the data in there
	# to be the key and data the same, but data is capitalized.
	puts "\tTest092.c: put/get data within a txn"
	set gflags ""
	if { [is_record_based $method] == 1 } {
		set checkfunc test092dr_recno.check
		append gflags " -recno"
	} else {
		set checkfunc test092dr.check
	}
	set count 0
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		set ustr [string toupper $str]
		set clret [list [list $key [pad_data $method $str]]]
		set drret [list [list $key [pad_data $method $ustr]]]
		#
		# Put the data in the txn.
		#
		set ret [eval {$dbtxn put} -txn $t \
		    {$key [chop_data $method $ustr]}]
		error_check_good put:$dbtxn $ret 0

		#
		# Now get the data using the different db handles and
		# make sure it is dirty or clean data.
		#
		# Using the dirty txn should show us dirty data
		set ret [eval {$dbcl get -txn $tdr} $gflags {$key}]
		error_check_good dbdr2:get $ret $drret

		set ret [eval {$dbdr get -dirty} $gflags {$key}]
		error_check_good dbdr1:get $ret $drret

		set ret [eval {$dbdr get -txn $tdr} $gflags {$key}]
		error_check_good dbdr2:get $ret $drret

		incr count
	}
	close $did

	puts "\tTest092.d: Check dirty data using dirty txn and clean db/cursor"
	dump_file_walk $dbccl $t1 $checkfunc "-first" "-next"

	puts "\tTest092.e: Check dirty data using -dirty cget flag"
	dump_file_walk $dbcdr0 $t2 $checkfunc "-first" "-next" "-dirty"

	puts "\tTest092.f: Check dirty data using -dirty cursor"
	dump_file_walk $dbcdr1 $t3 $checkfunc "-first" "-next"

	#
	# We must close these before aborting the real txn
	# because they all hold read locks on the pages.
	#
	error_check_good dbccl:close [$dbccl close] 0
	error_check_good dbcdr0:close [$dbcdr0 close] 0
	error_check_good dbcdr1:close [$dbcdr1 close] 0

	#
	# Now abort the modifying transaction and rerun the data checks.
	#
	puts "\tTest092.g: Aborting the write-txn"
	error_check_good txnabort [$t abort] 0

	set dbccl [$dbcl cursor -txn $tdr]
	error_check_good dbcurs:dbcl [is_valid_cursor $dbccl $dbcl] TRUE

	set dbcdr0 [$dbdr cursor]
	error_check_good dbcurs:dbdr0 [is_valid_cursor $dbcdr0 $dbdr] TRUE

	set dbcdr1 [$dbdr cursor -dirty]
	error_check_good dbcurs:dbdr1 [is_valid_cursor $dbcdr1 $dbdr] TRUE

	if { [is_record_based $method] == 1 } {
		set checkfunc test092cl_recno.check
	} else {
		set checkfunc test092cl.check
	}
	puts "\tTest092.h: Check clean data using -dirty cget flag"
	dump_file_walk $dbccl $t1 $checkfunc "-first" "-next"

	puts "\tTest092.i: Check clean data using -dirty cget flag"
	dump_file_walk $dbcdr0 $t2 $checkfunc "-first" "-next" "-dirty"

	puts "\tTest092.j: Check clean data using -dirty cursor"
	dump_file_walk $dbcdr1 $t3 $checkfunc "-first" "-next"

	# Clean up our handles
	error_check_good dbccl:close [$dbccl close] 0
	error_check_good tdrcommit [$tdr commit] 0
	error_check_good dbcdr0:close [$dbcdr0 close] 0
	error_check_good dbcdr1:close [$dbcdr1 close] 0
	error_check_good dbclose [$dbcl close] 0
	error_check_good dbclose [$dbdr close] 0
	error_check_good dbclose [$dbtxn close] 0
	error_check_good envclose [$env close] 0
}

# Check functions for test092; keys and data are identical
# Clean checks mean keys and data are identical.
# Dirty checks mean data are uppercase versions of keys.
proc test092cl.check { key data } {
	error_check_good "key/data mismatch" $key $data
}

proc test092cl_recno.check { key data } {
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}

proc test092dr.check { key data } {
	error_check_good "key/data mismatch" $key [string tolower $data]
}

proc test092dr_recno.check { key data } {
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data \
	    [string toupper $kvals($key)]
}

