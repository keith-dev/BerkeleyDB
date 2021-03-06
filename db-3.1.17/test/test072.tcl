# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: test072.tcl,v 11.6.2.1 2000/07/13 01:44:53 krinsky Exp $
#
# DB Test 72: Test that of cursor stability when duplicates are moved off-page.
proc test072 { method {pagesize 512} {ndups 20} {tnum 72} args } {
	source ./include.tcl
	global alphabet

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	cleanup $testdir
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test0$tnum.db
	} else {
		set testfile test0$tnum.db
	}

	# Keys must sort $prekey < $key < $postkey.
	set prekey "a key"
	set key "the key"
	set postkey "z key"

	# Make these distinguishable from each other and from the
	# alphabets used for the $key's data.
	set predatum "1234567890"
	set postdatum "0987654321"

	append args " -pagesize $pagesize -dup"

	puts -nonewline "Test0$tnum $omethod ($args): "
	if { [is_record_based $method] || [is_rbtree $method] } {
		puts "Skipping for method $method."
		return
	} else {
		puts "\n    Test of cursor stability when\
		    duplicates are moved off-page."
	}

	set db [eval {berkdb_open \
	     -create -truncate -mode 0644} $omethod $args $testfile]
	error_check_good "db open" [is_valid_db $db] TRUE

	puts "\tTest0$tnum.a: Set up surrounding keys and cursors."
	error_check_good pre_put [$db put $prekey $predatum] 0
	error_check_good post_put [$db put $postkey $postdatum] 0
	set precursor [$db cursor]
	error_check_good precursor [is_valid_cursor $precursor $db] TRUE
	set postcursor [$db cursor]
	error_check_good postcursor [is_valid_cursor $postcursor $db] TRUE
	error_check_good preset [$precursor get -set $prekey] \
		[list [list $prekey $predatum]]
	error_check_good postset [$postcursor get -set $postkey] \
		[list [list $postkey $postdatum]]

	puts "\tTest0$tnum.b: Put/create cursor/verify all cursor loop."

	for { set i 0 } { $i < $ndups } { incr i } {
		set datum [format "%4d$alphabet" [expr $i + 1000]]
		set data($i) $datum

		# Uncomment these lines to see intermediate steps.
		# error_check_good db_sync($i) [$db sync] 0
		# error_check_good db_dump($i) \
		#    [catch {exec ./db_dump -da $testfile > TESTDIR/out.$i}] 0

		error_check_good "db put ($i)" [$db put $key $datum] 0

		set dbc($i) [$db cursor]
		error_check_good "db cursor ($i)"\
		    [is_valid_cursor $dbc($i) $db] TRUE

		error_check_good "dbc get -get_both ($i)"\
		    [$dbc($i) get -get_both $key $datum]\
		    [list [list $key $datum]]

		for { set j 0 } { $j < $i } { incr j } {
		    set dbt [$dbc($j) get -current]
		    set k [lindex [lindex $dbt 0] 0]
		    set d [lindex [lindex $dbt 0] 1]

		    #puts "cursor $j after $i: $d"

		    eval {$db sync}

		    error_check_good\
			"cursor $j key correctness after $i puts" $k $key
		    error_check_good\
			"cursor $j data correctness after $i puts" $d $data($j)
		}

		# Check correctness of pre- and post- cursors.  Do an 
		# error_check_good on the lengths first so that we don't
		# spew garbage as the "got" field and screw up our
		# terminal.  (It's happened here.)
		set pre_dbt [$precursor get -current]
		set post_dbt [$postcursor get -current]
		error_check_good "earlier cursor correctness after $i puts" \
		    "key len [string length [lindex [lindex $pre_dbt 0] 0]]" \
		    "key len [string length $prekey]" 
		error_check_good "earlier cursor correctness after $i puts" \
		    "data len [string length [lindex [lindex $pre_dbt 0] 1]]" \
		    "data len [string length $predatum]" 
		error_check_good "later cursor correctness after $i puts" \
		    "key len [string length [lindex [lindex $post_dbt 0] 0]]" \
		    "key len [string length $postkey]" 
		error_check_good "later cursor correctness after $i puts" \
		    "data len [string length [lindex [lindex $post_dbt 0] 1]]"\
		    "data len [string length $postdatum]" 


		error_check_good "earlier cursor correctness after $i puts" \
		    $pre_dbt [list [list $prekey $predatum]]
		error_check_good "later cursor correctness after $i puts" \
		    $post_dbt [list [list $postkey $postdatum]]
	}

	# Close cursors.
	puts "\tTest0$tnum.c: Closing cursors."
	for { set i 0 } { $i < $ndups } { incr i } {
		error_check_good "dbc close ($i)" [$dbc($i) close] 0
	}
	error_check_good "db close" [$db close] 0
}
