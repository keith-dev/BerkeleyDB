/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ExceptionWrapperTest.java,v 12.2 2005/08/01 20:25:35 mark Exp $
 */

package com.sleepycat.util.test;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.collections.test.DbTestUtil;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.IOExceptionWrapper;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * @author Mark Hayes
 */
public class ExceptionWrapperTest extends TestCase {

    public static void main(String[] args)
        throws Exception {

        junit.framework.TestResult tr =
            junit.textui.TestRunner.run(suite());
        if (tr.errorCount() > 0 ||
            tr.failureCount() > 0) {
            System.exit(1);
        } else {
            System.exit(0);
        }
    }

    public static Test suite()
        throws Exception {

        TestSuite suite = new TestSuite(ExceptionWrapperTest.class);
        return suite;
    }

    public ExceptionWrapperTest(String name) {

        super(name);
    }

    public void setUp() {

        DbTestUtil.printTestName("ExceptionWrapperTest." + getName());
    }

    public void testIOWrapper()
        throws Exception {

        try {
            throw new IOExceptionWrapper(new RuntimeException("msg"));
        } catch (IOException e) {
            Exception ee = ExceptionUnwrapper.unwrap(e);
            assertTrue(ee instanceof RuntimeException);
            assertEquals("msg", ee.getMessage());

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof RuntimeException);
            assertEquals("msg", t.getMessage());
        }
    }

    public void testRuntimeWrapper()
        throws Exception {

        try {
            throw new RuntimeExceptionWrapper(new IOException("msg"));
        } catch (RuntimeException e) {
            Exception ee = ExceptionUnwrapper.unwrap(e);
            assertTrue(ee instanceof IOException);
            assertEquals("msg", ee.getMessage());

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof IOException);
            assertEquals("msg", t.getMessage());
        }
    }

    public void testErrorWrapper()
        throws Exception {

        try {
            throw new RuntimeExceptionWrapper(new Error("msg"));
        } catch (RuntimeException e) {
            try {
                ExceptionUnwrapper.unwrap(e);
                fail();
            } catch (Error ee) {
                assertTrue(ee instanceof Error);
                assertEquals("msg", ee.getMessage());
            }

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof Error);
            assertEquals("msg", t.getMessage());
        }
    }

    /**
     * Generates a stack trace for a nested exception and checks the output
     * for the nested exception.
     */
    public void testStackTrace() {

        /* Nested stack traces are not avilable in Java 1.3. */
        String version = System.getProperty("java.version");
        if (version.startsWith("1.3.")) {
            return;
        }

        Exception ex = new Exception("some exception");
        String causedBy = "Caused by: java.lang.Exception: some exception";

        try {
            throw new RuntimeExceptionWrapper(ex);
        } catch (RuntimeException e) {
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            String s = sw.toString();
            assertTrue(s.indexOf(causedBy) != -1);
        }

        try {
            throw new IOExceptionWrapper(ex);
        } catch (IOException e) {
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            String s = sw.toString();
            assertTrue(s.indexOf(causedBy) != -1);
        }
    }
}
