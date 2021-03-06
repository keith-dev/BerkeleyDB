# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: test057.tcl,v 11.16 2000/05/22 12:51:40 bostic Exp $
#
# Test057:
# Check if we handle the case where we delete a key with the cursor on it
# and then add the same key.  The cursor should not get the new item
# returned, but the item shouldn't disappear.
# Run test tests, one where the overwriting put is done with a put and
# one where it's done with a cursor put.
proc test057 { method args } {
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	append args " -create -truncate -mode 0644 -dup "
	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Test057: skipping for method $method"
		return
	}
	puts "Test057: $method delete and replace in presence of cursor."

	# Create the database and open the dictionary
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test057.db
	} else {
		set testfile test057.db
	}
	cleanup $testdir

	set flags ""
	set txn ""

	set db [eval {berkdb_open} $args {$omethod $testfile}]
	error_check_good dbopen:dup [is_valid_db $db] TRUE

	set curs [eval {$db cursor} $txn]
	error_check_good curs_open:dup [is_substr $curs $db] 1

	puts "\tTest057.a: Set cursor, delete cursor, put with key."
	# Put three keys in the database
	for { set key 1 } { $key <= 3 } {incr key} {
		set r [eval {$db put} $txn $flags {$key datum$key}]
		error_check_good put $r 0
	}

	# Retrieve keys sequentially so we can figure out their order
	set i 1
	for {set d [$curs get -first] } {[llength $d] != 0 } {\
			set d [$curs get -next] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	# Now put in a bunch of duplicates for key 2
	for { set d 1 } { $d <= 5 } {incr d} {
		set r [eval {$db put} $txn $flags {$key_set(2) dup_$d}]
		error_check_good dup:put $r 0
	}

	# Now put the cursor on key 1

	# Now set the cursor on the first of the duplicate set.
	set r [$curs get -set $key_set(1)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(1)
	error_check_good curs_get:DB_SET:data $d datum$key_set(1)

	# Now do the delete
	set r [$curs del]
	error_check_good delete $r 0

	# Now check the get current on the cursor.
	error_check_good curs_get:del [$curs get -current] [list [list [] []]]

	# Now do a put on the key
	set r [eval {$db put} $txn $flags {$key_set(1) new_datum$key_set(1)}]
	error_check_good put $r 0

	# Do a get
	set r [eval {$db get} $txn {$key_set(1)}]
	error_check_good get [lindex [lindex $r 0] 1] new_datum$key_set(1)

	# Recheck cursor
	error_check_good curs_get:deleted [$curs get -current] [list [list [] []]]

	# Move cursor and see if we get the key.
	set r [$curs get -first]
	error_check_bad cursor_get:DB_FIRST [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_FIRST:key $k $key_set(1)
	error_check_good curs_get:DB_FIRST:data $d new_datum$key_set(1)

	puts "\tTest057.b: Set two cursor on a key, delete one, overwrite other"
	set curs2 [eval {$db cursor} $txn]
	error_check_good curs2_open [is_substr $curs2 $db] 1

	# Set both cursors on the 4rd key
	set r [$curs get -set $key_set(3)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(3)
	error_check_good curs_get:DB_SET:data $d datum$key_set(3)

	set r [$curs2 get -set $key_set(3)]
	error_check_bad cursor2_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs2_get:DB_SET:key $k $key_set(3)
	error_check_good curs2_get:DB_SET:data $d datum$key_set(3)

	# Now delete through cursor 1
	error_check_good curs1_del [$curs del] 0

	# Verify gets on both 1 and 2
	error_check_good curs_get:deleted [$curs get -current] \
	    [list [list [] []]]
	error_check_good curs_get:deleted [$curs2 get -current] \
	    [list [list [] []]]

	# Now do a replace through cursor 2
	set pflags "-current"
	if {[is_hash $method] == 1} {
		error_check_good curs1_get_after_del [is_substr \
		    [$curs2 put $pflags new_datum$key_set(3)] "DB_NOTFOUND"] 1

		# Gets fail
		error_check_good curs1_get:deleted  \
		    [$curs get -current] \
		    [list [list [] []]]
		error_check_good curs2_get:deleted  \
		    [$curs get -current] \
		    [list [list [] []]]
	} else {
		# btree only, recno is skipped this test
		set ret [$curs2 put $pflags new_datum$key_set(3)]
		error_check_good curs_replace $ret 0
	}

	# Gets fail
	#error_check_good curs1_get:deleted [catch {$curs get -current} r] 1
	#error_check_good curs1_get_after_del \
	    [is_substr $errorInfo "DB_KEYEMPTY"] 1
	#error_check_good curs2_get:deleted [catch {$curs2 get -current} r] 1
	#error_check_good curs2_get_after_del \
	    [is_substr $errorInfo "DB_KEYEMPTY"] 1

	puts "\tTest057.c:\
	    Set two cursors on a dup, delete one, overwrite other"

	# Set both cursors on the 2nd duplicate of key 2
	set r [$curs get -set $key_set(2)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good curs_get:DB_SET:data $d datum$key_set(2)

	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_1

	set r [$curs2 get -set $key_set(2)]
	error_check_bad cursor2_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs2_get:DB_SET:key $k $key_set(2)
	error_check_good curs2_get:DB_SET:data $d datum$key_set(2)

	set r [$curs2 get -next]
	error_check_bad cursor2_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs2_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs2_get:DB_NEXT:data $d dup_1

	# Now delete through cursor 1
	error_check_good curs1_del [$curs del] 0

	# Verify gets on both 1 and 2
	error_check_good curs_get:deleted [$curs get -current] \
	    [list [list [] []]]
	error_check_good curs_get:deleted [$curs2 get -current] \
	    [list [list [] []]]

	# Now do a replace through cursor 2 -- this will work on btree but
	# not on hash
	if {[is_hash $method] == 1} {
		error_check_good hash_replace \
		    [is_substr [$curs2 put -current new_dup_1] "DB_NOTFOUND"] 1
	} else {
		error_check_good curs_replace [$curs2 put -current new_dup_1] 0
	}

	# Both gets should fail
	#error_check_good curs1_get:deleted [catch {$curs get -current} r] 1
	#error_check_good curs1_get_after_del \
	    [is_substr $errorInfo "DB_KEYEMPTY"] 1
	#error_check_good curs2_get:deleted [catch {$curs2 get -current} r] 1
	#error_check_good curs2_get_after_del \
	    [is_substr $errorInfo "DB_KEYEMPTY"] 1

	error_check_good curs2_close [$curs2 close] 0
	error_check_good curs_close [$curs close] 0
	error_check_good db_close [$db close] 0
}
