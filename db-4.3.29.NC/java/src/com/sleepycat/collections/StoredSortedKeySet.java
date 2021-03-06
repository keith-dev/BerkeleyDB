/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2004
 *      Sleepycat Software.  All rights reserved.
 *
 * $Id: StoredSortedKeySet.java,v 1.2 2004/06/02 20:59:39 mark Exp $
 */

package com.sleepycat.collections;

import java.util.Comparator;
import java.util.SortedSet;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.Database;

/**
 * The SortedSet returned by Map.keySet() and which can also be constructed
 * directly if a Map is not needed.
 * Since this collection is a set it only contains one element for each key,
 * even when duplicates are allowed.  Key set iterators are therefore
 * particularly useful for enumerating the unique keys of a store or index that
 * allows duplicates.
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
 * <p>In addition to the standard SortedSet methods, this class provides the
 * following methods for stored sorted sets only.  Note that the use of these
 * methods is not compatible with the standard Java collections interface.</p>
 * <ul>
 * <li>{@link #headSet(Object, boolean)}</li>
 * <li>{@link #tailSet(Object, boolean)}</li>
 * <li>{@link #subSet(Object, boolean, Object, boolean)}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public class StoredSortedKeySet extends StoredKeySet implements SortedSet {

    /**
     * Creates a sorted key set view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param keyBinding is the binding used to translate between key buffers
     * and key objects.
     *
     * @param writeAllowed is true to create a read-write collection or false
     * to create a read-only collection.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public StoredSortedKeySet(Database database, EntryBinding keyBinding,
                              boolean writeAllowed) {

        super(new DataView(database, keyBinding, null, null,
                           writeAllowed, null));
    }

    StoredSortedKeySet(DataView keySetView) {

        super(keySetView);
    }

    /**
     * Returns null since comparators are not supported.  The natural ordering
     * of a stored collection is data byte order, whether the data classes
     * implement the {@link java.lang.Comparable} interface or not.
     * This method does not conform to the {@link SortedSet#comparator}
     * interface.
     *
     * @return null.
     */
    public Comparator comparator() {

        return null;
    }

    /**
     * Returns the first (lowest) element currently in this sorted set.
     * This method conforms to the {@link SortedSet#first} interface.
     *
     * @return the first element.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public Object first() {

        return getFirstOrLast(true);
    }

    /**
     * Returns the last (highest) element currently in this sorted set.
     * This method conforms to the {@link SortedSet#last} interface.
     *
     * @return the last element.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public Object last() {

        return getFirstOrLast(false);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements are
     * strictly less than toKey.
     * This method conforms to the {@link SortedSet#headSet} interface.
     *
     * @param toKey is the upper bound.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet headSet(Object toKey) {

        return subSet(null, false, toKey, false);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements are
     * strictly less than toKey, optionally including toKey.
     * This method does not exist in the standard {@link SortedSet} interface.
     *
     * @param toKey is the upper bound.
     *
     * @param toInclusive is true to include toKey.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet headSet(Object toKey, boolean toInclusive) {

        return subSet(null, false, toKey, toInclusive);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements are
     * greater than or equal to fromKey.
     * This method conforms to the {@link SortedSet#tailSet} interface.
     *
     * @param fromKey is the lower bound.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet tailSet(Object fromKey) {

        return subSet(fromKey, true, null, false);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements are
     * strictly greater than fromKey, optionally including fromKey.
     * This method does not exist in the standard {@link SortedSet} interface.
     *
     * @param fromKey is the lower bound.
     *
     * @param fromInclusive is true to include fromKey.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet tailSet(Object fromKey, boolean fromInclusive) {

        return subSet(fromKey, fromInclusive, null, false);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements range
     * from fromKey, inclusive, to toKey, exclusive.
     * This method conforms to the {@link SortedSet#subSet} interface.
     *
     * @param fromKey is the lower bound.
     *
     * @param toKey is the upper bound.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet subSet(Object fromKey, Object toKey) {

        return subSet(fromKey, true, toKey, false);
    }

    /**
     * Returns a view of the portion of this sorted set whose elements are
     * strictly greater than fromKey and strictly less than toKey,
     * optionally including fromKey and toKey.
     * This method does not exist in the standard {@link SortedSet} interface.
     *
     * @param fromKey is the lower bound.
     *
     * @param fromInclusive is true to include fromKey.
     *
     * @param toKey is the upper bound.
     *
     * @param toInclusive is true to include toKey.
     *
     * @return the subset.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedSet subSet(Object fromKey, boolean fromInclusive,
                            Object toKey, boolean toInclusive) {

        try {
            return new StoredSortedKeySet(
               view.subView(fromKey, fromInclusive, toKey, toInclusive, null));
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }
}
