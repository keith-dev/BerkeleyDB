# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2001
#	Sleepycat Software.  All rights reserved.
#
# $Id: test068.tcl,v 11.14 2001/08/03 16:39:44 bostic Exp $
#
# TEST	test068
# TEST	Test of DB_BEFORE and DB_AFTER with partial puts.
# TEST	Make sure DB_BEFORE and DB_AFTER work properly with partial puts, and
# TEST	check that they return EINVAL if DB_DUPSORT is set or if DB_DUP is not.
proc test068 { method args } {
	source ./include.tcl
	global alphabet
	global errorCode

	set tnum 68
	set nkeys 1000

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test0$tnum.db
		set env NULL
	} else {
		set testfile test0$tnum.db
		incr eindex
		set env [lindex $args $eindex]
	}

	puts "Test0$tnum:\
	    $method ($args) Test of DB_BEFORE/DB_AFTER and partial puts."
	if { [is_record_based $method] == 1 } {
	    puts "\tTest0$tnum: skipping for method $method."
	    return
	}

	# Create a list of $nkeys words to insert into db.
	puts "\tTest0$tnum.a: Initialize word list."
	set wordlist {}
	set count 0
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nkeys } {
		lappend wordlist $str
		incr count
	}
	close $did

	# Sanity check:  did we get $nkeys words?
	error_check_good enough_keys [llength $wordlist] $nkeys

	# rbtree can't handle dups, so just test the non-dup case
	# if it's the current method.
	if { [is_rbtree $method] == 1 } {
		set dupoptlist { "" }
	} else {
		set dupoptlist { "" "-dup" "-dup -dupsort" }
	}

	foreach dupopt $dupoptlist {
		cleanup $testdir $env
		set db [eval {berkdb_open_noerr -create -mode 0644 \
		    $omethod} $args $dupopt {$testfile}]
		error_check_good db_open [is_valid_db $db] TRUE

		puts "\tTest0$tnum.b ($dupopt): DB initialization: put loop."
		foreach word $wordlist {
			error_check_good db_put [$db put $word $word] 0
		}

		puts "\tTest0$tnum.c ($dupopt): get loop."
		foreach word $wordlist {
			# Make sure that the Nth word has been correctly
			# inserted, and also that the Nth word is the
			# Nth one we pull out of the database using a cursor.

			set dbt [$db get $word]
			error_check_good get_key [list [list $word $word]] $dbt
		}

		set dbc [$db cursor]
		error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE

		puts "\tTest0$tnum.d ($dupopt): DBC->put w/ DB_AFTER."

		# Set cursor to the first key;  make sure it succeeds.
		# With an unsorted wordlist, we can't be sure that the
		# first item returned will equal the first item in the
		# wordlist, so we just make sure it got something back.
		set dbt [eval {$dbc get -first}]
		error_check_good \
		    dbc_get_first [llength $dbt] 1

		# If -dup is not set, or if -dupsort is set too, we
		# need to verify that DB_BEFORE and DB_AFTER fail
		# and then move on to the next $dupopt.
		if { $dupopt != "-dup" } {
			set errorCode "NONE"
			set ret [catch {eval $dbc put -after \
				{-partial [list 6 0]} "after"} res]
			error_check_good dbc_put_after_fail $ret 1
			error_check_good dbc_put_after_einval \
				[is_substr $errorCode EINVAL] 1
			puts "\tTest0$tnum ($dupopt): DB_AFTER returns EINVAL."
			set errorCode "NONE"
			set ret [catch {eval $dbc put -before \
				{-partial [list 6 0]} "before"} res]
			error_check_good dbc_put_before_fail $ret 1
			error_check_good dbc_put_before_einval \
				[is_substr $errorCode EINVAL] 1
			puts "\tTest0$tnum ($dupopt): DB_BEFORE returns EINVAL."
			puts "\tTest0$tnum ($dupopt): Correct error returns,\
				skipping further test."
			# continue with broad foreach
			error_check_good db_close [$db close] 0
			continue
		}

		puts "\tTest0$tnum.e ($dupopt): DBC->put(DB_AFTER) loop."
		foreach word $wordlist {
			# set cursor to $word
			set dbt [$dbc get -set $word]
			error_check_good \
			    dbc_get_set $dbt [list [list $word $word]]
			# put after it
			set ret [$dbc put -after -partial {4 0} after]
			error_check_good dbc_put_after $ret 0
		}

		puts "\tTest0$tnum.f ($dupopt): DBC->put(DB_BEFORE) loop."
		foreach word $wordlist {
			# set cursor to $word
			set dbt [$dbc get -set $word]
			error_check_good \
			    dbc_get_set $dbt [list [list $word $word]]
			# put before it
			set ret [$dbc put -before -partial {6 0} before]
			error_check_good dbc_put_before $ret 0
		}

		error_check_good dbc_close [$dbc close] 0

		eval $db sync
		puts "\tTest0$tnum.g ($dupopt): Verify correctness."

		set dbc [$db cursor]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		# loop through the whole db beginning to end,
		# make sure we have, in order, {$word "\0\0\0\0\0\0before"},
		# {$word $word}, {$word "\0\0\0\0after"} for each word.
		set count 0
		while { $count < $nkeys } {
			# Get the first item of each set of three.
			# We don't know what the word is, but set $word to
			# the key and check that the data is
			# "\0\0\0\0\0\0before".
			set dbt [$dbc get -next]
			set word [lindex [lindex $dbt 0] 0]

			error_check_good dbc_get_one $dbt \
			    [list [list $word "\0\0\0\0\0\0before"]]

			set dbt [$dbc get -next]
			error_check_good \
			    dbc_get_two $dbt [list [list $word $word]]

			set dbt [$dbc get -next]
			error_check_good dbc_get_three $dbt \
			    [list [list $word "\0\0\0\0after"]]

			incr count
		}
		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0
	}
}
