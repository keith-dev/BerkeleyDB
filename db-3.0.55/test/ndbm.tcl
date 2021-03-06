# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999
#	Sleepycat Software.  All rights reserved.
#
#	@(#)ndbm.tcl	11.4 (Sleepycat) 9/24/99
#
# Historic NDBM interface test.
# Use the first 1000 entries from the dictionary.
# Insert each with self as key and data; retrieve each.
# After all are entered, retrieve all; compare output to original.
# Then reopen the file, re-retrieve everything.
# Finally, delete everything.
proc ndbm { { nentries 1000 } } {
	source ./include.tcl

	puts "NDBM interfaces test: $nentries"

	# Create the database and open the dictionary
	set testfile $testdir/ndbmtest
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir

	set db [berkdb ndbm_open -create -truncate -mode 0644 $testfile]
	error_check_good ndbm_open [is_substr $db ndbm] 1
	set did [open $dict]

	error_check_good rdonly_false [$db rdonly] 0

	set flags 0
	set txn 0
	set count 0

	puts "\tNDBM.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		set ret [$db store $str $str insert]
		error_check_good ndbm_store $ret 0

		set d [$db fetch $str]
		error_check_good ndbm_fetch $d $str
		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tNDBM.b: dump file"
	set oid [open $t1 w]
	for { set key [$db firstkey] } { $key != -1 } {
	    set key [$db nextkey] } {
		puts $oid $key
		set d [$db fetch $key]
		error_check_good ndbm_refetch $d $key
	}
	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	set q q
	exec $SED $nentries$q $dict > $t3
	exec $SORT $t3 > $t2
	exec $SORT $t1 > $t3

	error_check_good NDBM:diff($t3,$t2) \
	    [catch { exec $CMP $t3 $t2 } res] 0

	puts "\tNDBM.c: pagf/dirf test"
	set fd [$db pagfno]
	error_check_bad pagf $fd -1
	set fd [$db dirfno]
	error_check_bad dirf $fd -1

	puts "\tNDBM.d: close, open, and dump file"

	# Now, reopen the file and run the last test again.
	error_check_good ndbm_close [$db close] 0
	set db [berkdb ndbm_open -rdonly $testfile]
	error_check_good ndbm_open2 [is_substr $db ndbm] 1
	set oid [open $t1 w]

	error_check_good rdonly_true [$db rdonly] "rdonly: not owner"

	for { set key [$db firstkey] } { $key != -1 } {
	    set key [$db nextkey] } {
		puts $oid $key
		set d [$db fetch $key]
		error_check_good ndbm_refetch2 $d $key
	}
	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	exec $SORT $t1 > $t3

	error_check_good NDBM:diff($t2,$t3) \
	    [catch { exec $CMP $t2 $t3 } res] 0

	# Now, reopen the file and delete each entry
	puts "\tNDBM.e: sequential scan and delete"

	error_check_good ndbm_close [$db close] 0
	set db [berkdb ndbm_open $testfile]
	error_check_good ndbm_open3 [is_substr $db ndbm] 1
	set oid [open $t1 w]

	for { set key [$db firstkey] } { $key != -1 } {
	    set key [$db nextkey] } {
		puts $oid $key
		set ret [$db delete $key]
		error_check_good ndbm_delete $ret 0
	}
	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	exec $SORT $t1 > $t3

	error_check_good NDBM:diff($t2,$t3) \
	    [catch { exec $CMP $t2 $t3 } res] 0
}
