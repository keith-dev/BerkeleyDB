# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999
#	Sleepycat Software.  All rights reserved.
#
#	@(#)test044.tcl	11.8 (Sleepycat) 10/25/99
#
# DB Test 40 {access method}
# System integration DB test: verify that locking, recovery, checkpoint,
# and all the other utilities basically work.
#
# The test consists of $nprocs processes operating on $nfiles files.  A
# transaction consists of adding the same key/data pair to some random
# number of these files.  We generate a bimodal distribution in key
# size with 70% of the keys being small (1-10 characters) and the
# remaining 30% of the keys being large (uniform distribution about
# mean $key_avg).  If we generate a key, we first check to make sure
# that the key is not already in the dataset.  If it is, we do a lookup.
#
# XXX This test uses grow-only files currently!
proc test044 { method {nprocs 5} {nfiles 10} {cont 0} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_rbtree $method] == 1 } {
		puts "Test044 skipping for method $method"
		return
	}

	puts "Test044: system integration test db $method $nprocs processes \
	    on $nfiles files"

	# Parse options
	set otherargs ""
	set key_avg 10
	set data_avg 20
	set do_exit 0
	for { set i 0 } { $i < [llength $args] } {incr i} {
		switch -regexp -- [lindex $args $i] {
			-key_avg { incr i; set key_avg [lindex $args $i] }
			-data_avg { incr i; set data_avg [lindex $args $i] }
			-testdir { incr i; set testdir [lindex $args $i] }
			-x.* { set do_exit 1 }
			default {
				lappend otherargs [lindex $args $i]
			}
		}
	}

	if { $cont == 0 } {
		# Create the database and open the dictionary
		cleanup $testdir

		# Create an environment
		puts "\tTest044.a: creating environment and $nfiles files"
		set dbenv [berkdb env \
		    -create -mpool -lock -log -txn -home $testdir]
		error_check_good env_open [is_valid_env $dbenv] TRUE

		# Create a bunch of files
		set m $method

		for { set i 0 } { $i < $nfiles } { incr i } {
			if { $method == "all" } {
				switch [berkdb random_int 1 2] {
				1 { set m -btree }
				2 { set m -hash }
				}
			} else {
				set m $omethod
			}

			set db [eval {berkdb open -env $dbenv -create \
			    -mode 0644 $m} $otherargs {test044.$i.db}]
			error_check_good dbopen [is_valid_db $db] TRUE
			error_check_good db_close [$db close] 0
		}
	}

	# Close the environment
	$dbenv close

	if { $do_exit == 1 } {
		return
	}

	# Database is created, now fork off the kids.
	puts "\tTest044.b: forking off $nprocs processes and utilities"
	set cycle 1
        set ncycles 3
	while { $cycle <= $ncycles } {
		set dbenv [berkdb env \
		    -create -mpool -lock -log -txn -home $testdir]
		error_check_good env_open [is_valid_env $dbenv] TRUE
		error_check_good env_close [$dbenv close] 0

		# Fire off deadlock detector and checkpointer
		puts "Beginning cycle $cycle"
		set ddpid [exec ./db_deadlock -h $testdir -t 5 &]
		set cppid [exec ./db_checkpoint -h $testdir -p 2 &]
		puts "Deadlock detector: $ddpid Checkpoint daemon $cppid"

		set pidlist {}
		for { set i 0 } {$i < $nprocs} {incr i} {
			set p [exec $tclsh_path \
			    $test_path/sysscript.tcl $testdir \
			    $nfiles $key_avg $data_avg $omethod \
			    > $testdir/test044.$i.log &]
			lappend pidlist $p
		}
		puts "[timestamp] $nprocs processes running $pidlist"
		exec $SLEEP [berkdb random_int 300 600]

		# Now simulate a crash
		puts "[timestamp] Crashing"
		exec $KILL -9 $ddpid
		exec $KILL -9 $cppid
		foreach p $pidlist {
			exec $KILL -9 $p
		}

		# Now run recovery
		test044_verify $testdir $nfiles
		incr cycle
	}
}

proc test044_usage { } {
	puts -nonewline "test044 method nentries [-d directory] [-i iterations]"
	puts " [-p procs] -x"
}

proc test044_verify { dir nfiles } {
	source ./include.tcl

	# Save everything away in case something breaks
#	for { set f 0 } { $f < $nfiles } {incr f} {
#		exec $CP $dir/test044.$f.db $dir/test044.$f.save1
#	}
#	foreach f [glob $dir/log.*] {
#		if { [is_substr $f save] == 0 } {
#			exec $CP $f $f.save1
#		}
#	}

	# Run recovery and then read through all the database files to make
	# sure that they all look good.

	puts "\tTest044.verify: Running recovery and verifying file contents"
	set stat [catch {exec ./db_recover -h $dir} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
	}

	# Save everything away in case something breaks
#	for { set f 0 } { $f < $nfiles } {incr f} {
#		exec $CP $dir/test044.$f.db $dir/test044.$f.save2
#	}
#	foreach f [glob $dir/log.*] {
#		if { [is_substr $f save] == 0 } {
#			exec $CP $f $f.save2
#		}
#	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		set db($f) [berkdb open $dir/test044.$f.db]
		error_check_good $f:dbopen [is_valid_db $db($f)] TRUE

		set cursors($f) [$db($f) cursor]
		error_check_bad $f:cursor_open $cursors($f) NULL
		error_check_good \
		    $f:cursor_open [is_substr $cursors($f) $db($f)] 1
	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		for {set d [$cursors($f) get -first] } \
		    { [string length $d] != 0 } \
		    { set d [$cursors($f) get -next] } {

			set k [lindex [lindex $d 0] 0]
			set d [lindex [lindex $d 0] 1]

			set flist [zero_list $nfiles]
			set r $d
			while { [set ndx [string first : $r]] != -1 } {
				set fnum [string range $r 0 [expr $ndx - 1]]
				if { [lindex $flist $fnum] == 0 } {
					set fl "-set"
				} else {
					set fl "-next"
				}

				if { $fl != "-set" || $fnum != $f } {
					if { [string compare $fl "-set"] == 0} {
						set full [$cursors($fnum) \
						    get -set $k]
					} else  {
						set full [$cursors($fnum) \
						    get -next]
					}
					set key [lindex [lindex $full 0] 0]
					set rec [lindex [lindex $full 0] 1]
					error_check_good \
					    $f:dbget_$fnum:key $key $k
					error_check_good \
					    $f:dbget_$fnum:data $rec $d
				}

				set flist [lreplace $flist $fnum $fnum 1]
				incr ndx
				set r [string range $r $ndx end]
			}
		}
	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		error_check_good $cursors($f) [$cursors($f) close] 0
		error_check_good db_close:$f [$db($f) close] 0
	}
}
