/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2005
 *      Sleepycat Software.  All rights reserved.
 *
 * $Id: StoredContainer.java,v 12.3 2005/08/01 20:25:19 mark Exp $
 */

package com.sleepycat.collections;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * A abstract base class for all stored collections and maps.  This class
 * provides implementations of methods that are common to the {@link
 * java.util.Collection} and the {@link java.util.Map} interfaces, namely
 * {@link #clear}, {@link #isEmpty} and {@link #size}.
 *
 * <p><em>Note that this class does not conform to the standard Java
 * collections interface in the following ways:</em></p>
 * <ul>
 * <li>The {@link #size} method always throws
 * <code>UnsupportedOperationException</code> because, for performance reasons,
 * databases do not maintain their total record count.</li>
 * <li>All iterators must be explicitly closed using {@link
 * StoredIterator#close()} or {@link StoredIterator#close(java.util.Iterator)}
 * to release the underlying database cursor resources.</li>
 * </ul>
 *
 * <p>In addition, this class provides the following methods for stored
 * collections only.  Note that the use of these methods is not compatible with
 * the standard Java collections interface.</p>
 * <ul>
 * <li>{@link #isWriteAllowed()}</li>
 * <li>{@link #isSecondary()}</li>
 * <li>{@link #isOrdered()}</li>
 * <li>{@link #areDuplicatesAllowed()}</li>
 * <li>{@link #areDuplicatesOrdered()}</li>
 * <li>{@link #areKeysRenumbered()}</li>
 * <li>{@link #getCursorConfig()}</li>
 * <li>{@link #isTransactional()}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public abstract class StoredContainer implements Cloneable {

    DataView view;

    StoredContainer(DataView view) {

        this.view = view;
    }

    /**
     * Returns true if this is a read-write container or false if this is a
     * read-only container.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * @return whether write is allowed.
     */
    public final boolean isWriteAllowed() {

        return view.writeAllowed;
    }

    /**
     * Returns the cursor configuration that is used for all operations
     * performed via this container.
     * For example, if <code>CursorConfig.getReadUncommitted</code> returns
     * true, data will be read that is modified but not committed.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * @return the cursor configuration, or null if no configuration has been
     * specified.
     */
    public final CursorConfig getCursorConfig() {

        return DbCompat.cloneCursorConfig(view.cursorConfig);
    }

    /**
     * Returns whether read-uncommitted is allowed for this container.
     * For the JE product, read-uncommitted is always allowed; for the DB
     * product, read-uncommitted is allowed if it was configured for the
     * underlying database for this container.
     * Even when read-uncommitted is allowed it must specifically be enabled by
     * calling one of the {@link StoredCollections} methods.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * @return whether read-uncommitted is allowed.
     *
     * @deprecated This method is deprecated with no replacement in this class.
     * In the DB product, <code>DatabaseConfig.getReadUncommitted</code> may be
     * called.
     */
    public final boolean isDirtyReadAllowed() {

        return view.readUncommittedAllowed;
    }

    /**
     * @deprecated This method has been replaced by {@link #getCursorConfig}.
     * <code>CursorConfig.isReadUncommitted</code> may be called to determine
     * whether dirty-read is enabled.
     */
    public final boolean isDirtyRead() {

        return view.cursorConfig.getReadUncommitted();
    }

    /**
     * Returns whether the databases underlying this container are
     * transactional.
     * Even in a transactional environment, a database will be transactional
     * only if it was opened within a transaction or if the auto-commit option
     * was specified when it was opened.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * @return whether the database is transactional.
     */
    public final boolean isTransactional() {

        return view.transactional;
    }

    /**
     * Clones a container with a specified cursor configuration.
     */
    final StoredContainer configuredClone(CursorConfig config) {

        try {
            StoredContainer cont = (StoredContainer) clone();
            cont.view = cont.view.configuredView(config);
            cont.initAfterClone();
            return cont;
        } catch (CloneNotSupportedException willNeverOccur) { return null; }
    }

    /**
     * Override this method to initialize view-dependent fields.
     */
    void initAfterClone() {
    }

    /**
     * Returns whether duplicate keys are allowed in this container.
     * Duplicates are optionally allowed for HASH and BTREE databases.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * <p>Note that the JE product only supports BTREE databases.</p>
     *
     * @return whether duplicates are allowed.
     */
    public final boolean areDuplicatesAllowed() {

        return view.dupsAllowed;
    }

    /**
     * Returns whether duplicate keys are allowed and sorted by element value.
     * Duplicates are optionally sorted for HASH and BTREE databases.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * <p>Note that the JE product only supports BTREE databases, and
     * duplicates are always sorted.</p>
     *
     * @return whether duplicates are ordered.
     */
    public final boolean areDuplicatesOrdered() {

        return view.dupsOrdered;
    }

    /**
     * Returns whether keys are renumbered when insertions and deletions occur.
     * Keys are optionally renumbered for RECNO databases.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * <p>Note that the JE product does not support RECNO databases, and
     * therefore keys are never renumbered.</p>
     *
     * @return whether keys are renumbered.
     */
    public final boolean areKeysRenumbered() {

        return view.keysRenumbered;
    }

    /**
     * Returns whether keys are ordered in this container.
     * Keys are ordered for BTREE, RECNO and QUEUE database.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * <p>Note that the JE product only support BTREE databases, and
     * therefore keys are always ordered.</p>
     *
     * @return whether keys are ordered.
     */
    public final boolean isOrdered() {

        return view.ordered;
    }

    /**
     * Returns whether this container is a view on a secondary database rather
     * than directly on a primary database.
     * This method does not exist in the standard {@link java.util.Map} or
     * {@link java.util.Collection} interfaces.
     *
     * @return whether the view is for a secondary database.
     */
    public final boolean isSecondary() {

        return view.isSecondary();
    }

    /**
     * Always throws UnsupportedOperationException.  The size of a database
     * cannot be obtained reliably or inexpensively.
     * This method therefore violates the {@link java.util.Collection#size} and
     * {@link java.util.Map#size} interfaces.
     *
     * @return always throws an exception.
     *
     * @throws UnsupportedOperationException unconditionally.
     */
    public int size() {

        throw new UnsupportedOperationException(
            "collection size not available");
    }

    /**
     * Returns true if this map or collection contains no mappings or elements.
     * This method conforms to the {@link java.util.Collection#isEmpty} and
     * {@link java.util.Map#isEmpty} interfaces.
     *
     * @return whether the container is empty.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is thrown.
     */
    public boolean isEmpty() {

        try {
            return view.isEmpty();
        } catch (Exception e) {
            throw convertException(e);
        }
    }

    /**
     * Removes all mappings or elements from this map or collection (optional
     * operation).
     * This method conforms to the {@link java.util.Collection#clear} and
     * {@link java.util.Map#clear} interfaces.
     *
     * @throws UnsupportedOperationException if the container is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is thrown.
     */
    public void clear() {

        boolean doAutoCommit = beginAutoCommit();
        try {
            view.clear();
            commitAutoCommit(doAutoCommit);
        } catch (Exception e) {
            throw handleException(e, doAutoCommit);
        }
    }

    Object get(Object key) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            if (OperationStatus.SUCCESS ==
                cursor.getSearchKey(key, null, false)) {
                return cursor.getCurrentValue();
            } else {
                return null;
            }
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    Object put(final Object key, final Object value) {

        DataCursor cursor = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            cursor = new DataCursor(view, true);
            Object[] oldValue = new Object[1];
            cursor.put(key, value, oldValue, false);
            closeCursor(cursor);
            commitAutoCommit(doAutoCommit);
            return oldValue[0];
        } catch (Exception e) {
            closeCursor(cursor);
            throw handleException(e, doAutoCommit);
        }
    }

    final boolean removeKey(final Object key, final Object[] oldVal) {

        DataCursor cursor = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            cursor = new DataCursor(view, true);
            boolean found = false;
            OperationStatus status = cursor.getSearchKey(key, null, true);
            while (status == OperationStatus.SUCCESS) {
                cursor.delete();
                found = true;
                if (oldVal != null && oldVal[0] == null) {
                    oldVal[0] = cursor.getCurrentValue();
                }
                status = areDuplicatesAllowed() ?
                    cursor.getNextDup(true): OperationStatus.NOTFOUND;
            }
            closeCursor(cursor);
            commitAutoCommit(doAutoCommit);
            return found;
        } catch (Exception e) {
            closeCursor(cursor);
            throw handleException(e, doAutoCommit);
        }
    }

    boolean containsKey(Object key) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            return OperationStatus.SUCCESS ==
                   cursor.getSearchKey(key, null, false);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    final boolean removeValue(Object value) {

        DataCursor cursor = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            cursor = new DataCursor(view, true);
            OperationStatus status = cursor.find(value, true);
            if (status == OperationStatus.SUCCESS) {
                cursor.delete();
            }
            closeCursor(cursor);
            commitAutoCommit(doAutoCommit);
            return (status == OperationStatus.SUCCESS);
        } catch (Exception e) {
            closeCursor(cursor);
            throw handleException(e, doAutoCommit);
        }
    }

    boolean containsValue(Object value) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            OperationStatus status = cursor.find(value, true);
            return (status == OperationStatus.SUCCESS);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    final void closeCursor(DataCursor cursor) {

        if (cursor != null) {
            try {
                cursor.close();
            } catch (Exception e) {
                throw StoredContainer.convertException(e);
            }
        }
    }

    final boolean beginAutoCommit() {

        if (view.transactional) {
            CurrentTransaction currentTxn = view.getCurrentTxn();
            try {
                if (currentTxn.isAutoCommitAllowed()) {
                    currentTxn.beginTransaction(null);
                    return true;
                }
            } catch (DatabaseException e) {
                throw new RuntimeExceptionWrapper(e);
            }
        }
        return false;
    }

    final void commitAutoCommit(boolean doAutoCommit)
        throws DatabaseException {

        if (doAutoCommit) view.getCurrentTxn().commitTransaction();
    }

    final RuntimeException handleException(Exception e, boolean doAutoCommit) {

        if (doAutoCommit) {
            try {
                view.getCurrentTxn().abortTransaction();
            } catch (DatabaseException ignored) {
		/* Klockwork - ok */
            }
        }
        return StoredContainer.convertException(e);
    }

    static RuntimeException convertException(Exception e) {

        if (e instanceof RuntimeException) {
            return (RuntimeException) e;
        } else {
            return new RuntimeExceptionWrapper(e);
        }
    }
}
