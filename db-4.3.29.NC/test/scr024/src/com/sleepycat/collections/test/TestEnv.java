/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2004
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: TestEnv.java,v 1.3 2004/06/30 01:08:22 mark Exp $
 */

package com.sleepycat.collections.test;

import java.io.File;
import java.io.IOException;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

/**
 * @author Mark Hayes
 */
public class TestEnv {

    public static final TestEnv BDB;
    public static final TestEnv CDB;
    public static final TestEnv TXN;
    static {
        EnvironmentConfig config;

        config = newEnvConfig();
        BDB = new TestEnv("bdb", config);

        if (DbCompat.CDB) {
            config = newEnvConfig();
            DbCompat.setInitializeCDB(config, true);
            CDB = new TestEnv("cdb", config);
        } else {
            CDB = null;
        }

        config = newEnvConfig();
        config.setTransactional(true);
        DbCompat.setInitializeLocking(config, true);
        TXN = new TestEnv("txn", config);
    }

    private static EnvironmentConfig newEnvConfig() {

        EnvironmentConfig config = new EnvironmentConfig();
        if (DbCompat.MEMORY_SUBSYSTEM) {
            DbCompat.setInitializeCache(config, true);
        }
        return config;
    }

    public static final TestEnv[] ALL;
    static {
        if (DbCompat.CDB) {
            ALL = new TestEnv[] { BDB, CDB, TXN };
        } else {
            ALL = new TestEnv[] { BDB, TXN };
        }
    }

    private String name;
    private EnvironmentConfig config;

    private TestEnv(String name, EnvironmentConfig config) {

        this.name = name;
        this.config = config;
    }

    public String getName() {

        return name;
    }

    public boolean isTxnMode() {

        return config.getTransactional();
    }

    public boolean isCdbMode() {

        return DbCompat.getInitializeCDB(config);
    }

    public Environment open(String testName)
        throws IOException, DatabaseException {

        return open(testName, true);
    }

    public Environment open(String testName, boolean create)
        throws IOException, DatabaseException {

        config.setAllowCreate(create);
        /* OLDEST deadlock detection on DB matches the use of timeouts on JE.*/
        DbCompat.setLockDetectModeOldest(config);
        File dir = getDirectory(testName, create);
        return new Environment(dir, config);
    }

    public File getDirectory(String testName)
        throws IOException {

        return getDirectory(testName, true);
    }

    public File getDirectory(String testName, boolean create)
        throws IOException {

        if (create) {
            return DbTestUtil.getNewDir("db-test/" + testName);
        } else {
            return DbTestUtil.getExistingDir("db-test/" + testName);
        }
    }
}
