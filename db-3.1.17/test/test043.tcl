# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: test043.tcl,v 11.11 2000/04/21 18:36:26 krinsky Exp $
#
# DB Test 43 {method nentries}
# Test the Record number implicit creation and renumbering options.
proc test043 { method {nentries 10000} args} {
	source ./include.tcl

	set do_renumber [is_rrecno $method]
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test043: $method ($args)"

	if { [is_record_based $method] != 1 } {
		puts "Test043 skipping for method $method"
		return
	}

	# Create the database and open the dictionary
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test043.db
	} else {
		set testfile test043.db
	}
	cleanup $testdir

	# Create the database
	set db [eval {berkdb_open -create -truncate -mode 0644} $args \
		{$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags " -recno"
	set txn ""

	# First test implicit creation and retrieval
	set count 1
	set interval 5
	if { $nentries < $interval } {
		set nentries [expr $interval + 1]
	}
	puts "\tTest043.a: insert keys at $interval record intervals"
	while { $count <= $nentries } {
		set ret [eval {$db put} \
		    $txn $pflags {$count [chop_data $method $count]}]
		error_check_good "$db put $count" $ret 0
		set last $count
		incr count $interval
	}

	puts "\tTest043.b: get keys using DB_FIRST/DB_NEXT"
	set dbc [eval {$db cursor} $txn]
	error_check_good "$db cursor" [is_substr $dbc $db] 1

	set check 1
	for { set rec [$dbc get -first] } { [llength $rec] != 0 } {
	    set rec [$dbc get -next] } {
		set k [lindex [lindex $rec 0] 0]
		set d [pad_data $method [lindex [lindex $rec 0] 1]]
		error_check_good "$dbc get key==data" [pad_data $method $k] $d
		error_check_good "$dbc get sequential" $k $check
		if { $k > $nentries } {
			error_check_good "$dbc get key too large" $k $nentries
		}
		incr check $interval
	}

	# Now make sure that we get DB_KEYEMPTY for non-existent keys
	puts "\tTest043.c: Retrieve non-existent keys"
	global errorInfo

	set check 1
	for { set rec [$dbc get -first] } { [llength $rec] != 0 } {
		set rec [$dbc get -next] } {
		set k [lindex [lindex $rec 0] 0]

		set ret [eval {$db get} $txn $gflags {[expr $k + 1]}]
		error_check_good "$db \
		    get [expr $k + 1]" $ret [list]

		incr check $interval
		# Make sure we don't do a retrieve past the end of file
		if { $check >= $last }  {
			break
		}
	}

	# Now try deleting and make sure the right thing happens.
	puts "\tTest043.d: Delete tests"
	set rec [$dbc get -first]
	error_check_bad "$dbc get -first" [llength $rec] 0
	error_check_good  "$dbc get -first key" [lindex [lindex $rec 0] 0] 1
	error_check_good  "$dbc get -first data" \
	    [lindex [lindex $rec 0] 1] [pad_data $method 1]

	# Delete the first item
	error_check_good "$dbc del" [$dbc del] 0

	# Retrieving 1 should always fail
	set ret [eval {$db get} $txn $gflags {1}]
	error_check_good "$db get 1" $ret [list]

	# Now, retrieving other keys should work; keys will vary depending
	# upon renumbering.
	if { $do_renumber == 1 } {
		set count [expr 0 + $interval]
		set max [expr $nentries - 1]
	} else {
		set count [expr 1 + $interval]
		set max $nentries
	}

	while { $count <= $max } {
	set rec [eval {$db get} $txn $gflags {$count}]
		if { $do_renumber == 1 } {
			set data [expr $count + 1]
		} else {
			set data $count
		}
		error_check_good "$db get $count" \
		    [pad_data $method $data] [lindex [lindex $rec 0] 1]
		incr count $interval
	}
	set max [expr $count - $interval]

	puts "\tTest043.e: Verify LAST/PREV functionality"
	set count $max
	for { set rec [$dbc get -last] } { [llength $rec] != 0 } {
	    set rec [$dbc get -prev] } {
		set k [lindex [lindex $rec 0] 0]
		set d [lindex [lindex $rec 0] 1]
		if { $do_renumber == 1 } {
			set data [expr $k + 1]
		} else {
			set data $k
		}
		error_check_good \
		    "$dbc get key==data" [pad_data $method $data] $d
		error_check_good "$dbc get sequential" $k $count
		if { $k > $nentries } {
			error_check_good "$dbc get key too large" $k $nentries
		}
		set count [expr $count - $interval]
		if { $count < 1 } {
			break
		}
	}
	error_check_good dbc_close [$dbc close] 0
	error_check_good db_close [$db close] 0
}
