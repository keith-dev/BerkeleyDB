# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2001
#	Sleepycat Software.  All rights reserved.
#
# $Id: test073.tcl,v 11.21 2001/08/03 16:39:45 bostic Exp $
#
# TEST	test073
# TEST	Test of cursor stability on duplicate pages.
# TEST
# TEST	Does the following:
# TEST	a. Initialize things by DB->putting ndups dups and
# TEST	   setting a reference cursor to point to each.
# TEST	b. c_put ndups dups (and correspondingly expanding
# TEST	   the set of reference cursors) after the last one, making sure
# TEST	   after each step that all the reference cursors still point to
# TEST	   the right item.
# TEST	c. Ditto, but before the first one.
# TEST	d. Ditto, but after each one in sequence first to last.
# TEST	e. Ditto, but after each one in sequence from last to first.
# TEST	   occur relative to the new datum)
# TEST	f. Ditto for the two sequence tests, only doing a
# TEST	   DBC->c_put(DB_CURRENT) of a larger datum instead of adding a
# TEST	   new one.
proc test073 { method {pagesize 512} {ndups 50} {tnum 73} args } {
	source ./include.tcl
	global alphabet

	set omethod [convert_method $method]
	set args [convert_args $method $args]

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
	cleanup $testdir $env

	set key "the key"

	puts -nonewline "Test0$tnum $omethod ($args): "
	if { [is_record_based $method] || [is_rbtree $method] } {
		puts "Skipping for method $method."
		return
	} else {
		puts "cursor stability on duplicate pages."
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test073: skipping for specific pagesizes"
		return
	}

	append args " -pagesize $pagesize -dup"

	set db [eval {berkdb_open \
	     -create -mode 0644} $omethod $args $testfile]
	error_check_good "db open" [is_valid_db $db] TRUE

	# Number of outstanding keys.
	set keys 0

	puts "\tTest0$tnum.a.1: Initializing put loop; $ndups dups, short data."

	for { set i 0 } { $i < $ndups } { incr i } {
		set datum [makedatum_t73 $i 0]

		error_check_good "db put ($i)" [$db put $key $datum] 0

		set is_long($i) 0
		incr keys
	}

	puts "\tTest0$tnum.a.2: Initializing cursor get loop; $keys dups."
	for { set i 0 } { $i < $keys } { incr i } {
		set datum [makedatum_t73 $i 0]

		set dbc($i) [$db cursor]
		error_check_good "db cursor ($i)"\
		    [is_valid_cursor $dbc($i) $db] TRUE
		error_check_good "dbc get -get_both ($i)"\
		    [$dbc($i) get -get_both $key $datum]\
		    [list [list $key $datum]]
	}

	puts "\tTest0$tnum.b: Cursor put (DB_KEYLAST); $ndups new dups,\
	    short data."

	for { set i 0 } { $i < $ndups } { incr i } {
		# !!! keys contains the number of the next dup
		# to be added (since they start from zero)

		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE
		error_check_good "c_put(DB_KEYLAST, $keys)"\
		    [$curs put -keylast $key $datum] 0

		set dbc($keys) $curs
		set is_long($keys) 0
		incr keys

		verify_t73 is_long dbc $keys $key
	}

	puts "\tTest0$tnum.c: Cursor put (DB_KEYFIRST); $ndups new dups,\
	    short data."

	for { set i 0 } { $i < $ndups } { incr i } {
		# !!! keys contains the number of the next dup
		# to be added (since they start from zero)

		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE
		error_check_good "c_put(DB_KEYFIRST, $keys)"\
		    [$curs put -keyfirst $key $datum] 0

		set dbc($keys) $curs
		set is_long($keys) 0
		incr keys

		verify_t73 is_long dbc $keys $key
	}

	puts "\tTest0$tnum.d: Cursor put (DB_AFTER) first to last;\
	    $keys new dups, short data"
	# We want to add a datum after each key from 0 to the current
	# value of $keys, which we thus need to save.
	set keysnow $keys
	for { set i 0 } { $i < $keysnow } { incr i } {
		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		# Which datum to insert this guy after.
		set curdatum [makedatum_t73 $i 0]
		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $curdatum]\
		    [list [list $key $curdatum]]
		error_check_good "c_put(DB_AFTER, $i)"\
		    [$curs put -after $datum] 0

		set dbc($keys) $curs
		set is_long($keys) 0
		incr keys

		verify_t73 is_long dbc $keys $key
	}

	puts "\tTest0$tnum.e: Cursor put (DB_BEFORE) last to first;\
	    $keys new dups, short data"

	for { set i [expr $keys - 1] } { $i >= 0 } { incr i -1 } {
		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		# Which datum to insert this guy before.
		set curdatum [makedatum_t73 $i 0]
		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $curdatum]\
		    [list [list $key $curdatum]]
		error_check_good "c_put(DB_BEFORE, $i)"\
		    [$curs put -before $datum] 0

		set dbc($keys) $curs
		set is_long($keys) 0
		incr keys

		if { $i % 10 == 1 } {
			verify_t73 is_long dbc $keys $key
		}
	}
	verify_t73 is_long dbc $keys $key

	puts "\tTest0$tnum.f: Cursor put (DB_CURRENT), first to last,\
	    growing $keys data."
	set keysnow $keys
	for { set i 0 } { $i < $keysnow } { incr i } {
		set olddatum [makedatum_t73 $i 0]
		set newdatum [makedatum_t73 $i 1]
		set curs [$db cursor]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $olddatum]\
		    [list [list $key $olddatum]]
		error_check_good "c_put(DB_CURRENT, $i)"\
		    [$curs put -current $newdatum] 0

		error_check_good "cursor close" [$curs close] 0

		set is_long($i) 1

		if { $i % 10 == 1 } {
			verify_t73 is_long dbc $keys $key
		}
	}
	verify_t73 is_long dbc $keys $key

	# Close cursors.
	puts "\tTest0$tnum.g: Closing cursors."
	for { set i 0 } { $i < $keys } { incr i } {
		error_check_good "dbc close ($i)" [$dbc($i) close] 0
	}
	error_check_good "db close" [$db close] 0
}

# !!!: This procedure is also used by test087.
proc makedatum_t73 { num is_long } {
	global alphabet
	if { $is_long == 1 } {
		set a $alphabet$alphabet$alphabet
	} else {
		set a abcdefghijklm
	}

	# format won't do leading zeros, alas.
	if { $num / 1000 > 0 } {
		set i $num
	} elseif { $num / 100 > 0 } {
		set i 0$num
	} elseif { $num / 10 > 0 } {
		set i 00$num
	} else {
		set i 000$num
	}

	return $i$a
}

# !!!: This procedure is also used by test087.
proc verify_t73 { is_long_array curs_array numkeys key } {
	upvar $is_long_array is_long
	upvar $curs_array dbc
	upvar db db

	#useful for debugging, perhaps.
	eval $db sync

	for { set j 0 } { $j < $numkeys } { incr j } {
		set dbt [$dbc($j) get -current]
		set k [lindex [lindex $dbt 0] 0]
		set d [lindex [lindex $dbt 0] 1]

		error_check_good\
		    "cursor $j key correctness (with $numkeys total items)"\
		    $k $key
		error_check_good\
		    "cursor $j data correctness (with $numkeys total items)"\
		    $d [makedatum_t73 $j $is_long($j)]
	}
}
