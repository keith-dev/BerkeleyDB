/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2004
 *      Sleepycat Software.  All rights reserved.
 *
 * $Id: StoredValueSet.java,v 1.3 2004/06/04 18:24:50 mark Exp $
 */

package com.sleepycat.collections;

import java.util.Set;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.OperationStatus;

/**
 * The Set returned by Map.values() and Map.duplicates(), and which can also be
 * constructed directly if a Map is not needed.
 * Although this collection is a set it may contain duplicate values.  Only if
 * an entity value binding is used are all elements guaranteed to be unique.
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
 * @author Mark Hayes
 */
public class StoredValueSet extends StoredCollection implements Set {

    /*
     * This class is also used internally for the set returned by duplicates().
     */

    private boolean isSingleKey;

    /**
     * Creates a value set view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param valueBinding is the binding used to translate between value
     * buffers and value objects.
     *
     * @param writeAllowed is true to create a read-write collection or false
     * to create a read-only collection.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public StoredValueSet(Database database,
                          EntryBinding valueBinding,
                          boolean writeAllowed) {

        super(new DataView(database, null, valueBinding, null,
                           writeAllowed, null));
    }

    /**
     * Creates a value set entity view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param valueEntityBinding is the binding used to translate between
     * key/value buffers and entity value objects.
     *
     * @param writeAllowed is true to create a read-write collection or false
     * to create a read-only collection.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public StoredValueSet(Database database,
                          EntityBinding valueEntityBinding,
                          boolean writeAllowed) {

        super(new DataView(database, null, null, valueEntityBinding,
                           writeAllowed, null));
    }

    StoredValueSet(DataView valueSetView) {

        super(valueSetView);
    }

    StoredValueSet(DataView valueSetView, boolean isSingleKey) {

        super(valueSetView);
        this.isSingleKey = isSingleKey;
    }

    /**
     * Adds the specified entity to this set if it is not already present
     * (optional operation).
     * This method conforms to the {@link Set#add} interface.
     *
     * @param entity is the entity to be added.
     *
     * @return true if the entity was added, that is the key-value pair
     * represented by the entity was not previously present in the collection.
     *
     * @throws UnsupportedOperationException if the collection is read-only,
     * if the collection is indexed, or if an entity binding is not used.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean add(Object entity) {

        if (view.isSecondary()) {
            throw new UnsupportedOperationException(
                "add() not allowed with index");
        } else if (isSingleKey) {
            /* entity is actually just a value in this case */
            if (!view.dupsAllowed) {
                throw new UnsupportedOperationException("duplicates required");
            }
            DataCursor cursor = null;
            boolean doAutoCommit = beginAutoCommit();
            try {
                cursor = new DataCursor(view, true);
                cursor.useRangeKey();
                OperationStatus status =
                    cursor.putNoDupData(null, entity, null, true);
                closeCursor(cursor);
                commitAutoCommit(doAutoCommit);
                return (status == OperationStatus.SUCCESS);
            } catch (Exception e) {
                closeCursor(cursor);
                throw handleException(e, doAutoCommit);
            }
        } else if (view.entityBinding == null) {
            throw new UnsupportedOperationException(
                "add() requires entity binding");
        } else {
            return add(null, entity);
        }
    }

    /**
     * Returns true if this set contains the specified element.
     * This method conforms to the {@link java.util.Set#contains}
     * interface.
     *
     * @param value the value to check.
     *
     * @return whether the set contains the given value.
     */
    public boolean contains(Object value) {

        return containsValue(value);
    }

    /**
     * Removes the specified value from this set if it is present (optional
     * operation).
     * If an entity binding is used, the key-value pair represented by the
     * given entity is removed.  If an entity binding is used, the first
     * occurrence of a key-value pair with the given value is removed.
     * This method conforms to the {@link Set#remove} interface.
     *
     * @throws UnsupportedOperationException if the collection is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean remove(Object value) {

        return removeValue(value);
    }

    // javadoc is inherited
    public int size() {

        if (!isSingleKey) {
	    return super.size();
	}
        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            OperationStatus status = cursor.getFirst(false);
            if (status == OperationStatus.SUCCESS) {
                return cursor.count();
            } else {
                return 0;
            }
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    Object makeIteratorData(StoredIterator iterator, DataCursor cursor)
        throws DatabaseException {

        return cursor.getCurrentValue();
    }

    boolean hasValues() {

        return true;
    }
}
