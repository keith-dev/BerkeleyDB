/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2004
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: Part.java,v 1.2 2004/09/22 16:17:13 mark Exp $
 */

package com.sleepycat.examples.collections.ship.tuple;

/**
 * A Part represents the combined key/data pair for a part entity.
 *
 * <p> In this sample, Part is created from the stored key/data entry using a
 * SerialSerialBinding.  See {@link SampleViews.PartBinding} for details.
 * Since this class is not directly used for data storage, it does not need to
 * be Serializable. </p>
 *
 * @author Mark Hayes
 */
public class Part {

    private String number;
    private String name;
    private String color;
    private Weight weight;
    private String city;

    public Part(String number, String name, String color, Weight weight,
                String city) {

        this.number = number;
        this.name = name;
        this.color = color;
        this.weight = weight;
        this.city = city;
    }

    public final String getNumber() {

        return number;
    }

    public final String getName() {

        return name;
    }

    public final String getColor() {

        return color;
    }

    public final Weight getWeight() {

        return weight;
    }

    public final String getCity() {

        return city;
    }

    public String toString() {

        return "[Part: number=" + number +
               " name=" + name +
               " color=" + color +
               " weight=" + weight +
               " city=" + city + ']';
    }
}
