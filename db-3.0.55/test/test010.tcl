# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999
#	Sleepycat Software.  All rights reserved.
#
#	@(#)test010.tcl	11.5 (Sleepycat) 9/24/99
#
# DB Test 10 {access method}
# Use the first 10,000 entries from the dictionary.
# Insert each with self as key and data; add duplicate
# records for each.
# After all are entered, retrieve all; verify output.
# Close file, reopen, do retrieve and re-verify.
# This does not work for recno
proc test010 { method {nentries 10000} {ndups 5} {tnum 10} args } {
	source ./include.tcl

	set omethod $method
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test0$tnum skipping for method $method"
		return
	}

	puts "Test0$tnum: $method ($args) $nentries small dup key/data pairs"

	# Create the database and open the dictionary
	set testfile $testdir/test0$tnum.db
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	cleanup $testdir

	set db [eval {berkdb \
	    open -create -truncate -mode 0644 -dup} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	set dbc [eval {$db cursor} $txn]
	while { [gets $did str] != -1 && $count < $nentries } {
		for { set i 1 } { $i <= $ndups } { incr i } {
			set datastr $i:$str
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
		}

		# Now retrieve all the keys matching this key
		set x 1
		for {set ret [$dbc get "-set" $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get "-next"] } {
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			if {[string length $d] == 0} {
				break
			}
			error_check_good "Test0$tnum:put" $d $str
			set id [ id_of $datastr ]
			error_check_good "Test0$tnum:dup#" $id $x
			incr x
		}
		error_check_good "Test0$tnum:ndups:$str" [expr $x - 1] $ndups
		incr count
	}
	error_check_good cursor_close [$dbc close] 0
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest0$tnum.a: Checking file for correct duplicates"
	set dlist ""
	for { set i 1 } { $i <= $ndups } {incr i} {
		lappend dlist $i
	}
	dup_check $db $txn $t1 $dlist

	# Now compare the keys to see if they match the dictionary entries
	set q q
	exec $SED $nentries$q $dict > $t3
	exec $SORT $t3 > $t2
	exec $SORT $t1 > $t3

	error_check_good Test0$tnum:diff($t3,$t2) \
	    [catch { exec $CMP $t3 $t2 } res] 0

	error_check_good db_close [$db close] 0
	set db [berkdb open $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest0$tnum.b: Checking file for correct duplicates after close"
	dup_check $db $txn $t1 $dlist

	# Now compare the keys to see if they match the dictionary entries
	exec $SORT $t1 > $t3
	error_check_good Test0$tnum:diff($t3,$t2) \
	    [catch { exec $CMP $t3 $t2 } res] 0

	error_check_good db_close [$db close] 0
}
