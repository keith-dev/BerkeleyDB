/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: Weight.java,v 12.4 2006/08/24 14:46:00 bostic Exp $
 */

package collections.ship.sentity;

import java.io.Serializable;

/**
 * Weight represents a weight amount and unit of measure.
 *
 * <p> In this sample, Weight is embedded in part data values which are stored
 * as Java serialized objects; therefore Weight must be Serializable. </p>
 *
 * @author Mark Hayes
 */
public class Weight implements Serializable {

    public final static String GRAMS = "grams";
    public final static String OUNCES = "ounces";

    private double amount;
    private String units;

    public Weight(double amount, String units) {

        this.amount = amount;
        this.units = units;
    }

    public final double getAmount() {

        return amount;
    }

    public final String getUnits() {

        return units;
    }

    public String toString() {

        return "[" + amount + ' ' + units + ']';
    }
}
