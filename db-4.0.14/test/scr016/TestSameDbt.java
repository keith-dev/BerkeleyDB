/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: TestSameDbt.java,v 1.2 2001/10/05 02:36:10 bostic Exp $
 */

/*
 * Simple test for get/put of specific values.
 */

package com.sleepycat.test;

import com.sleepycat.db.*;
import java.io.FileNotFoundException;

public class TestSameDbt
{
    public static void main(String[] args)
    {
        try {
            Db db = new Db(null, 0);
            db.open("my.db", null, Db.DB_BTREE, Db.DB_CREATE, 0644);

            // try reusing the dbt
            Dbt keydatadbt = new Dbt("stuff".getBytes());
            int gotexcept = 0;

            try {
                db.put(null, keydatadbt, keydatadbt, 0);
            }
            catch (DbException dbe) {
                System.out.println("got expected Db Exception: " + dbe);
                gotexcept++;
            }

            if (gotexcept != 1) {
                System.err.println("Missed exception");
                System.out.println("** FAIL **");
            }
            else {
                System.out.println("Test succeeded.");
            }
        }
        catch (DbException dbe) {
            System.err.println("Db Exception: " + dbe);
        }
        catch (FileNotFoundException fnfe) {
            System.err.println("FileNotFoundException: " + fnfe);
        }

    }

}
