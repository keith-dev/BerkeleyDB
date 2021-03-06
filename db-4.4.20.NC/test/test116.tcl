# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005
#       Sleepycat Software.  All rights reserved.
#
# $Id: test116.tcl,v 12.2 2005/10/12 18:30:38 sue Exp $
#
# TEST	test116
# TEST	Test of basic functionality of lsn_reset.
# TEST
# TEST	Create a database in an env.  Copy it to a new file within
# TEST	the same env.  Reset the page LSNs.
proc test116 { method {tnum "116"} args } {
	source ./include.tcl
	global util_path
	global passwd

	puts "Test$tnum ($method): Test lsn_reset."

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]

	set testfile A.db
	set newfile B.db
	set nentries 50
	set filenames "A B C D E"

	# This test needs two envs.  If one is provided, create the 
	# second under it.  If no env is provided, create both. 
	set txn ""
	set txnenv 0
	set envargs ""
	set resetargs ""
	set eindex [lsearch -exact $args "-env"]

	if { $eindex == -1 } {
		puts "\tTest$tnum.a: Creating env."
		set env [eval {berkdb_env} \
		    -create $encargs $envargs -home $testdir -txn]
		append args " -auto_commit "
		error_check_good dbenv [is_valid_env $env] TRUE
	} else {
		incr eindex
		set env [lindex $args $eindex]
		puts "\tTest$tnum.a: Using provided env $env."

		# Make sure the second env we create has all the
		# same flags the provided env does.
		if { [is_substr [$env get_open_flags] "-thread"] } {
			append envargs " -thread "
		}
		if { [is_substr $args "-encrypt"] } {
			append envargs " -encryptaes $passwd "
		}
		if { [is_substr [$env get_encrypt_flags] "-encryptaes"] } {
			append envargs " -encryptaes $passwd "
			append resetargs " -encrypt "
		}
		set txn ""
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		} elseif { $txnenv == 0 } {
			puts "Skipping Test$tnum for non-transactional env."
			return
		}
	 	set testdir [get_home $env] 
	}

	foreach lorder { 1234 4321 } {
		# Open database A, populate and close.
		puts "\tTest$tnum.b: Creating database with lorder $lorder."
	 	cleanup $testdir $env

		# Create a second directory, and create an env there. 
		set testdir [get_home $env]
		set newdir $testdir/NEWDIR
		file mkdir $newdir
		set newenv [eval {berkdb_env} \
		    -create $encargs $envargs -home $newdir -txn]
		error_check_good newenv [is_valid_env $newenv] TRUE

		# We test with subdatabases except with the queue access
		# method, where they are not allowed.
		if { [is_queue $method] == 1 } {
			set db [eval {berkdb_open} -env $env \
			    $omethod $args -create -mode 0644 $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE
			set pgsize [stat_field $db stat "Page size"]	
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			for { set i 1 } { $i <= $nentries } { incr i } {
				set key $i
				set data DATA.$i
				error_check_good db_put [eval {$db put} \
				    $txn $key [chop_data $method $data]] 0
			}
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			error_check_good db_close [$db close] 0
		} else {
			foreach filename $filenames {
				set db [eval {berkdb_open} -env $env \
				    $omethod $args -create \
				    -mode 0644 $testfile $filename]
				error_check_good dbopen [is_valid_db $db] TRUE
				set pgsize [stat_field $db stat "Page size"]	
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good \
					    txn [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				for { set i 1 } { $i <= $nentries } { incr i } {
					set key $i
					set data DATA.$i
					error_check_good \
					    db_put [eval {$db put} $txn \
					    $key [chop_data $method $data]] 0
				}
				if { $txnenv == 1 } {
					error_check_good t_commit [$t commit] 0
				}		
				error_check_good db_close [$db close] 0
			}
		}	
	
		# Copy database file A.  Reset LSNs on the copy. Then 
		# test that the copy is usable both in its native env
		# and in a new env. 

		puts "\tTest$tnum.c: Copy database and reset its LSNs."
		set testdir [get_home $env]
		set newdir [get_home $newenv]

		# Reset LSNs before copying. 
		file copy -force $testdir/$testfile $testdir/$newfile
		error_check_good \
		    lsn_reset [eval {$env lsn_reset} $resetargs {$newfile}] 0

		file copy -force $testdir/$newfile $newdir/$testfile

		# If we're using queue extents, we must copy the extents.
		if { [is_queueext $method] } {
			set cwd [pwd]
			cd $testdir
			set extents [glob __dbq.*]
			foreach extent $extents {
				set idx [string last . $extent] 
				incr idx
				set suffix [string range $extent $idx end]
				file copy -force $extent __dbq.$newfile.$suffix
				file copy -force \
				    $extent NEWDIR/__dbq.$testfile.$suffix
			}
			cd $cwd
		}

		# Get the LSNs and compare them.
		set npages [getlsns $testdir/$testfile $pgsize orig_lsns]
		set newpages [getlsns $testdir/$newfile $pgsize new_lsns]
		set newdirpages [getlsns $newdir/$testfile $pgsize newdir_lsns]
		error_check_good newpages_match $npages $newpages
		error_check_good newdirpages_match $npages $newdirpages
		for { set i 0 } { $i < $npages } { incr i } {
			error_check_bad \
			    newlsns_match $orig_lsns($i) $new_lsns($i)
			error_check_bad \
			    newdirlsns_match $orig_lsns($i) $newdir_lsns($i)
		}

		puts "\tTest$tnum.d: Verify directories with reset LSNs."
	 	error_check_good \
		    verify [verify_dir $testdir "\tTest$tnum.d: "] 0
	 	error_check_good \
		    verify [verify_dir $newdir "\tTest$tnum.e: "] 0
	
		puts "\tTest$tnum.f: Open new db, check data, close db."
		if { [is_queue $method] == 1 } {
			set db [eval {berkdb_open} -env $newenv \
			    $omethod $args -create -mode 0644 $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE
			if { $txnenv == 1 } {
				set t [$newenv txn]
				error_check_good txn [is_valid_txn $t $newenv] TRUE
				set txn "-txn $t"
			}
			for { set i 1 } { $i <= $nentries } { incr i } {
				set key $i
				set ret [eval {$db get} $txn $key]
				error_check_good db_get \
				    [lindex [lindex $ret 0] 1] \
				    [pad_data $method DATA.$i]
			}
			if { $txnenv == 1 } {
				error_check_good txn_commit [$t commit] 0
			}
			error_check_good db_close [$db close] 0
		} else {
			foreach filename $filenames {
				set db [eval {berkdb_open} \
				    -env $newenv $omethod $args \
				    -create -mode 0644 $testfile $filename ]
				error_check_good dbopen [is_valid_db $db] TRUE
				if { $txnenv == 1 } {
					set t [$newenv txn]
					error_check_good \
					    txn [is_valid_txn $t $newenv] TRUE
					set txn "-txn $t"
				}
				for { set i 1 } { $i <= $nentries } { incr i } {
					set key $i
					set ret [eval {$db get} $txn $key]
					error_check_good db_get \
					    [lindex [lindex $ret 0] 1] \
					    [pad_data $method DATA.$i]
				}
				if { $txnenv == 1 } {
					error_check_good txn_commit [$t commit] 0
				}		
				error_check_good db_close [$db close] 0
			}
		}
		error_check_good newenv_close [$newenv close] 0 
	}

	# Close the parent env if this test created it. 
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}

proc getlsns { dbfile pgsize lsns } {
	upvar $lsns file_lsns
	set fid [open $dbfile r]
	fconfigure $fid -translation binary
	set eof 0
	set pg 0
	while { $eof == 0 } {
		set offset [expr $pg * $pgsize]
		seek $fid $offset start
		set file_lsns($pg) [read $fid 8]
		set eof [eof $fid]
		incr pg
	}
	close $fid
	incr pg -1
	return $pg
}
