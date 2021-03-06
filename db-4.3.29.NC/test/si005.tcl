# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2004
#	Sleepycat Software.  All rights reserved.
#
# $Id: si005.tcl,v 11.11 2004/10/27 20:40:25 carol Exp $
#
# TEST	si005
# TEST	Basic secondary index put/delete test with transactions
proc si005 { methods {nentries 200} {tnum "005"} args } {
	source ./include.tcl
	global dict nsecondaries

	# Primary method/args.
	set pmethod [lindex $methods 0]
	set pargs [convert_args $pmethod $args]
	set pomethod [convert_method $pmethod]

	# Method/args for all the secondaries.  If only one method
	# was specified, assume the same method and a standard N
	# secondaries.
	set methods [lrange $methods 1 end]
	if { [llength $methods] == 0 } {
		for { set i 0 } { $i < $nsecondaries } { incr i } {
			lappend methods $pmethod
		}
	}

	# Since this is a transaction test, don't allow nentries to be large.
	if { $nentries > 1000 } {
		puts "Skipping si005 for large lists (over 1000 items)."
		return
	}

	set argses [convert_argses $methods $args]
	set omethods [convert_methods $methods]

	puts "si$tnum \{\[ list $pmethod $methods \]\} $nentries" 
	puts "\twith transactions"
	env_cleanup $testdir

	set pname "primary$tnum.db"
	set snamebase "secondary$tnum"

	# Open an environment
	# XXX if one is not supplied!
	set env [berkdb_env -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open the primary.
	set pdb [eval {berkdb_open -create -auto_commit -env} $env $pomethod \
	    $pargs $pname]
	error_check_good primary_open [is_valid_db $pdb] TRUE

	# Open and associate the secondaries
	set sdbs {}
	for { set i 0 } { $i < [llength $omethods] } { incr i } {
		set sdb [eval {berkdb_open -create -auto_commit -env} $env \
		    [lindex $omethods $i] [lindex $argses $i] $snamebase.$i.db]
		error_check_good second_open($i) [is_valid_db $sdb] TRUE

		error_check_good db_associate($i) \
		    [$pdb associate -auto_commit [callback_n $i] $sdb] 0
		lappend sdbs $sdb
	}

	puts "\tSi$tnum.a: Put loop"
	set did [open $dict]
	for { set n 0 } { [gets $did str] != -1 && $n < $nentries } { incr n } {
		if { [is_record_based $pmethod] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}
		set keys($n) $key
		set data($n) [pad_data $pmethod $datum]

		set txn [$env txn]
		set ret [eval {$pdb put} -txn $txn \
		    {$key [chop_data $pmethod $datum]}]
		error_check_good put($n) $ret 0
		error_check_good txn_commit($n) [$txn commit] 0
	}
	close $did
	check_secondaries $pdb $sdbs $nentries keys data "Si$tnum.a"

	puts "\tSi$tnum.b: Put/overwrite loop"
	for { set n 0 } { $n < $nentries } { incr n } {
		set newd $data($n).$keys($n)

		set txn [$env txn]
		set ret [eval {$pdb put} -txn $txn \
		    {$keys($n) [chop_data $pmethod $newd]}]
		error_check_good put_overwrite($n) $ret 0
		set data($n) [pad_data $pmethod $newd]
		error_check_good txn_commit($n) [$txn commit] 0
	}
	check_secondaries $pdb $sdbs $nentries keys data "Si$tnum.b"

	# Delete the second half of the entries through the primary.
	# We do the second half so we can just pass keys(0 ... n/2)
	# to check_secondaries.
	set half [expr $nentries / 2]
	puts "\tSi$tnum.c: Primary delete loop: deleting $half entries"
	for { set n $half } { $n < $nentries } { incr n } {
		set txn [$env txn]
		set ret [$pdb del -txn $txn $keys($n)]
		error_check_good pdel($n) $ret 0
		error_check_good txn_commit($n) [$txn commit] 0
	}
	check_secondaries $pdb $sdbs $half keys data "Si$tnum.c"

	# Delete half of what's left, through the first secondary.
	set quar [expr $half / 2]
	puts "\tSi$tnum.d: Secondary delete loop: deleting $quar entries"
	set sdb [lindex $sdbs 0]
	set callback [callback_n 0]
	for { set n $quar } { $n < $half } { incr n } {
		set skey [$callback $keys($n) [pad_data $pmethod $data($n)]]
		set txn [$env txn]
		set ret [$sdb del -txn $txn $skey]
		error_check_good sdel($n) $ret 0
		error_check_good txn_commit($n) [$txn commit] 0
	}
	check_secondaries $pdb $sdbs $quar keys data "Si$tnum.d"

	puts "\tSi$tnum.e: Closing/disassociating primary first"
	error_check_good primary_close [$pdb close] 0
	foreach sdb $sdbs {
		error_check_good secondary_close [$sdb close] 0
	}
	error_check_good env_close [$env close] 0
}
