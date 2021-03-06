# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2001
#	Sleepycat Software.  All rights reserved.
#
# $Id: test096.tcl,v 11.9 2001/08/03 16:39:49 bostic Exp $
#
# TEST	test096
# TEST	Db->truncate test.
proc test096 { method {pagesize 512} {nentries 1000} {ndups 19} args} {
	global fixed_len
	source ./include.tcl

	set orig_fixed_len $fixed_len
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test096: $method db truncate method test"
	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test096 skipping for method $method"
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test096: Skipping for specific pagesizes"
		return
	}

	# Create the database and open the dictionary
	set eindex [lsearch -exact $args "-env"]
	set testfile test096.db
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		#
		# Make sure the env we were given supports txns.
		#
		set stat [catch {$env txn} txn]
		if { $stat != 0 } {
			puts "Environment w/o txns specified;  skipping."
			return
		}
		error_check_good txnabort [$txn abort] 0
		set closeenv 0
	} else {
		env_cleanup $testdir

		#
		# We need an env for exclusive-use testing.
		set env [berkdb env -create -home $testdir -txn]
		error_check_good env_create [is_valid_env $env] TRUE
		set closeenv 1
	}

	set t1 $testdir/t1

	puts "\tTest096.a: Create $nentries entries"
	set db [eval {berkdb_open_noerr -create \
	    -env $env $omethod -mode 0644} $args $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	set did [open $dict]
	set count 0
	set txn ""
	set pflags ""
	set gflags ""
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str
		set datastr [reverse $str]
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $gflags {$key}]
		error_check_good $key:dbget [llength $ret] 1

		incr count
	}
	close $did

	puts "\tTest096.b: Truncate database"
	error_check_good dbclose [$db close] 0
	set dbtr [eval {berkdb_open_noerr -create \
	    -env $env $omethod -mode 0644} $args $testfile]
	error_check_good db_open [is_valid_db $dbtr] TRUE

	set ret [$dbtr truncate]
	error_check_good dbtrunc $ret $nentries
	error_check_good db_close [$dbtr close] 0

	set db [berkdb_open -env $env $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [$db get -glob *]
	error_check_good dbget [llength $ret] 0
	error_check_good dbclose [$db close] 0
	error_check_good dbverify [verify_dir $testdir "\tTest096.c: "] 0

	#
	# Remove database, and create a new one with dups.
	#
	puts "\tTest096.d: Create $nentries entries with $ndups duplicates"
	set ret [berkdb dbremove -env $env $testfile]
	set db [eval {berkdb_open_noerr -pagesize $pagesize -dup -create \
	    -env $env $omethod -mode 0644} $args $testfile]
	error_check_good db_open [is_valid_db $db] TRUE
	set did [open $dict]
	set count 0
	set txn ""
	set pflags ""
	set gflags ""
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str
		for { set i 1 } { $i <= $ndups } { incr i } {
			set datastr $i:$str
			set ret [eval {$db put} \
			    $txn $pflags {$key [chop_data $method $datastr]}]
			error_check_good put $ret 0
		}

		set ret [eval {$db get} $gflags {$key}]
		error_check_bad $key:dbget_dups [llength $ret] 0
		error_check_good $key:dbget_dups1 [llength $ret] $ndups

		incr count
	}
	close $did
	set dlist ""
	for { set i 1 } {$i <= $ndups} {incr i} {
		lappend dlist $i
	}
	dup_check $db $txn $t1 $dlist
	puts "\tTest096.e: Verify off page duplicates status"
	set stat [$db stat]
	error_check_bad stat:offpage [is_substr $stat \
	    "{{Duplicate pages} 0}"] 1

	set recs [expr $ndups * $count]
	error_check_good dbclose [$db close] 0

	puts "\tTest096.f: Truncate database in a txn then abort"
	set txn [$env txn]

	set dbtr [eval {berkdb_open_noerr -create \
	    -env $env $omethod -mode 0644} $args $testfile]
	error_check_good db_open [is_valid_db $dbtr] TRUE
	error_check_good txnbegin [is_valid_txn $txn $env] TRUE

	set ret [$dbtr truncate -txn $txn]
	error_check_good dbtrunc $ret $recs

	error_check_good txnabort [$txn abort] 0
	error_check_good db_close [$dbtr close] 0

	set db [berkdb_open -env $env $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [$db get -glob *]
	error_check_good dbget [llength $ret] $recs
	error_check_good dbclose [$db close] 0

	puts "\tTest096.g: Truncate database in a txn then commit"
	set txn [$env txn]
	error_check_good txnbegin [is_valid_txn $txn $env] TRUE

	set dbtr [eval {berkdb_open_noerr -create \
	    -env $env $omethod -mode 0644} $args $testfile]
	error_check_good db_open [is_valid_db $dbtr] TRUE

	set ret [$dbtr truncate -txn $txn]
	error_check_good dbtrunc $ret $recs

	error_check_good db_close [$dbtr close] 0
	error_check_good txncommit [$txn commit] 0

	set db [berkdb_open -env $env $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [$db get -glob *]
	error_check_good dbget [llength $ret] 0
	error_check_good dbclose [$db close] 0

	error_check_good dbverify [verify_dir $testdir "\tTest096.h: "] 0

	if { $closeenv == 1 } {
		error_check_good envclose [$env close] 0
	}
}
