/******************************************************************************
 * LoadCount.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import java.util.Date;
import java.util.LinkedList;
import java.text.DecimalFormat;

/**
 * Thread-safe class to monitor load count. Displays:
 *
 * <ul>
 * <li>...
 * </ul>
 */
public class LoadCount
{
    public class LoadInfo
    {
        // number of requests processed
        public int m01Requests = 0;
        public int m05Requests = 0;
        public int m15Requests = 0;

        // request avg. (requests / second)
        public float m01RequestAvg = 0;
        public float m05RequestAvg = 0;
        public float m15RequestAvg = 0;

        // avg. request time
        public float m01AvgResTime = 0;
        public float m05AvgResTime = 0;
        public float m15AvgResTime = 0;

        // max. request time
        public float m01MaxResTime = 0;
        public float m05MaxResTime = 0;
        public float m15MaxResTime = 0;

        // active requests
        public int mActiveRequestCount = 0;

        // total count for current bucket
        public int mCurrentRequestCount = 0;
        public int mCurrentResponseCount = 0;
        public int mCurrentResponseTimes = 0;

        // total request/response count since object instantiation
        public int mTotalRequestCount = 0;
        public int mTotalResponseCount = 0;
    }


    private static final int SECONDS_PER_MINUTE = 60;
    private static final int LOAD_01_MINUTES = 1;
    private static final int LOAD_05_MINUTES = 5;
    private static final int LOAD_15_MINUTES = 15;


    /** Display pattern to apply for floats. */
    private DecimalFormat mFormat = new DecimalFormat("#.##");

    /** The time when the next sample snapshot should happen. */
    private long mNext;

    /** Count of active request count. */
    private int mActiveRequestCount;

    /** Total request count since object creation. */
    private int mTotalRequestCount;

    /** Total response count since object creation. */
    private int mTotalResponseCount;

    /** Count of current sample load count. */
    private int mCurrentRequestCount;

    /** Each bucket represents requests per sample. */
    private int[] mRequestCount;

    /** Sum of current request times (in milliseconds). */
    private int mCurrentResponseTimes;

    /** Each bucket represents the sum of request times (in milliseconds) for a
     * sample interval. */
    private int[] mResponseTimes;

    /** Current max response time (in milliseconds). */
    private int mCurrentMaxResponseTime;

    /** Each bucket represents the max response time (in milliseconds) for a
     * sample interval. */
    private int[] mMaxResponseTime;

    /** Sum of current request times (in milliseconds). */
    private int mCurrentResponseCount;

    /** Each bucket represents the sum of request times (in milliseconds) per
     * sample interval. */
    private int[] mResponseCount;

    /** Location of array bucket to place the current sample count. */ 
    private int mPtr;

    /** Max number of sample slots for array. */
    private int mMaxSamples;

    /** Seconds per sample. */
    private int mSecsPerSample;

    /** Millisconds per sample. */
    private int mMilliSecsPerSample;

    /** Number of buckets used by 1 minute. */
    private int m01;

    /** Number of buckets used by 5 minutes. */
    private int m05;

    /** Number of buckets used by 15 minutes. */
    private int m15;

    /**
     * Construct a load count with 60 second sampling.
     */
    public LoadCount()
    {
        this(SECONDS_PER_MINUTE);
    }

    /**
     * Construct a load count with a user-defined sampling interval. If the
     * sampling interval is not divisble by 60 seconds, the following values are
     * used:
     *
     * <ul>
     *   <li>less than 1 => default to 15 secs</li>
     *   <li>greater than a minute => default to 60 secs</li>
     *   <li>not divisible into 60 secs => find the next higher divisible 
     *       number</li>
     * </ul>
     *
     * @param secsPerSample the time granularity on how often a load sample
     * should be taken.
     */
    public LoadCount(int secsPerSample)
    {
        // Make sure sample interval is a divisor of 60 seconds.
        if (secsPerSample < 1) {
            // less than 1 => default to 15 secs
            secsPerSample = 15;
        } else if (SECONDS_PER_MINUTE < secsPerSample) {
            // greater than a minute => default to 60 secs
            secsPerSample = SECONDS_PER_MINUTE;
        } else if (SECONDS_PER_MINUTE % secsPerSample != 0) {
            // not divisible into 60 secs => find the next higher divisible
            // number
            while (SECONDS_PER_MINUTE % ++secsPerSample != 0 )
                ;
        }

        mSecsPerSample = secsPerSample;
        mMilliSecsPerSample = mSecsPerSample * 1000;

        // N sample slots for 15 minutes.
        mMaxSamples = ( LOAD_15_MINUTES * SECONDS_PER_MINUTE ) / mSecsPerSample;
        mRequestCount = new int[mMaxSamples];
        mResponseCount = new int[mMaxSamples];
        mResponseTimes = new int[mMaxSamples];
        mMaxResponseTime = new int[mMaxSamples];

        m01 = ( LOAD_01_MINUTES * SECONDS_PER_MINUTE) / mSecsPerSample;
        m05 = ( LOAD_05_MINUTES * SECONDS_PER_MINUTE) / mSecsPerSample;
        m15 = ( LOAD_15_MINUTES * SECONDS_PER_MINUTE) / mSecsPerSample;

        // reset all values
        reset();
    }


    /**
     * Clear and reset all values.
     */
    synchronized public void reset()
    {
        mNext = new Date().getTime() + mSecsPerSample;
        mPtr = 0;
        mActiveRequestCount = 0;
        mCurrentRequestCount = 0;
        mCurrentResponseTimes = 0;
        mCurrentResponseCount = 0;
        mCurrentMaxResponseTime = 0;
        mTotalRequestCount = 0;
        mTotalResponseCount = 0;

        for (int i=0; i < mMaxSamples; i++) {
            mRequestCount[i] = 0;
            mResponseCount[i] = 0;
            mResponseTimes[i] = 0;
            mMaxResponseTime[i] = 0;
        }
    }

    /**
     * Check if it's time to count the next sample interval.
     */
    private void check()
    {
        long now = new Date().getTime();
        if (mNext <= now) {

            // Ptr to current ptr.
            int prevPtr = mPtr;

            // (1) find out how many sample intervals it's been. at least one
            // interval has elapsed or we wouldn't be in here.
            int n = (int) ( ( now - mNext ) / mMilliSecsPerSample ) + 1;

            // (2.a) calculate and place request time avg
            // (2.b) add last count to sample bucket
            // (2.c) check if we have to wrap around arrays
            mRequestCount[mPtr] = mCurrentRequestCount;
            mResponseCount[mPtr] = mCurrentResponseCount;
            mResponseTimes[mPtr] = mCurrentResponseTimes;
            mMaxResponseTime[mPtr] = mCurrentMaxResponseTime;
            if (--mPtr < 0)
                mPtr = mMaxSamples - 1;

            // (3) zero out rest of buckets during silent interval
            int count = n-1;
            while (0 < count--) {
                mRequestCount[mPtr] = 0;
                mResponseCount[mPtr] = 0;
                mResponseTimes[mPtr] = 0;
                mMaxResponseTime[mPtr] = 0;

                // We've zeroed out everything. Don't need to clear things more
                // than once.
                if (prevPtr == mPtr)
                    break;

                if (--mPtr < 0)
                    mPtr = mMaxSamples -1;
            }

            // (4) set next time we need to snap
            mNext += ( mMilliSecsPerSample * n );

            // (5) clear count
            mCurrentRequestCount = 0;
            mCurrentResponseCount = 0;
            mCurrentResponseTimes = 0;

            // (6) Clear max response time
            mCurrentMaxResponseTime = 0;
        }
    }


    /**
     * Increment load count for the current sample interval giving it a time.
     * @param t time it took for request to finish.
     */
    synchronized public void decrement(int t)
    {
        check();
        --mActiveRequestCount;
        ++mCurrentResponseCount;
        mCurrentResponseTimes += t;
        ++mTotalResponseCount;

        if (t > mCurrentMaxResponseTime) {
            mCurrentMaxResponseTime = t;
        }
    }

    /**
     * Increment load count for the current sample interval.
     */
    synchronized public void increment()
    {
        check();
        ++mActiveRequestCount;
        ++mCurrentRequestCount;
        ++mTotalRequestCount;
    }

    synchronized public LoadInfo getLoadInfo()
    {
        check();

        LoadInfo li = new LoadInfo();

        int p = mPtr+1;
        int n = 0;
        int reqCountSum = 0;
        int resCountSum = 0;
        int resTimeSum = 0;

        int maxResponseTime = 0;

        while ( n++ < m15 ) {
            p %= m15;
            reqCountSum += mRequestCount[p];
            resCountSum += mResponseCount[p];
            resTimeSum  += mResponseTimes[p];
            if (mMaxResponseTime[p] > maxResponseTime) {
                maxResponseTime = mMaxResponseTime[p];
            }
            p++;
            if (n == m01) {
                li.m01Requests = reqCountSum;
                if (reqCountSum != 0)
                    li.m01RequestAvg = (float)reqCountSum / (float) (LOAD_01_MINUTES * SECONDS_PER_MINUTE);
                if (resCountSum != 0) 
                    li.m01AvgResTime = (float)resTimeSum / (float) resCountSum;
                li.m01MaxResTime = maxResponseTime;
            }
            if (n == m05) {
                li.m05Requests = reqCountSum;
                if (reqCountSum != 0)
                    li.m05RequestAvg = (float)reqCountSum / (float) (LOAD_05_MINUTES * SECONDS_PER_MINUTE);
                if (resCountSum != 0)
                    li.m05AvgResTime = (float)resTimeSum / (float) resCountSum;
                li.m05MaxResTime = maxResponseTime;
            }
            if (n == m15) {
                li.m15Requests = reqCountSum;
                if (reqCountSum != 0)
                    li.m15RequestAvg = (float)reqCountSum / (float) (LOAD_15_MINUTES * SECONDS_PER_MINUTE);
                if (resCountSum != 0)
                    li.m15AvgResTime = (float)resTimeSum / (float) resCountSum;
                li.m15MaxResTime = maxResponseTime;
            }
        }

        li.mActiveRequestCount = mActiveRequestCount;
        li.mCurrentRequestCount = mCurrentRequestCount;
        li.mCurrentResponseCount = mCurrentResponseCount;
        li.mCurrentResponseTimes = mCurrentResponseTimes;

        li.mTotalRequestCount = mTotalRequestCount;
        li.mTotalResponseCount = mTotalResponseCount;

        return li;
    }

    /**
     * Return load information in XML w/o sample interval view.
     * @param tag tag name
     */
    public String toXML(String tag)
    {
        return toXML(tag, false);
    }

    /**
     * Return load information in XML.
     * @param tag tag name
     * @param verbose if true, returns information with all samples.
     */
    synchronized public String toXML(String tag, boolean verbose)
    {
        StringBuffer buf = new StringBuffer();
        LoadInfo load = getLoadInfo();

        // req: total requests for interval
        // rps: requests per second during interval
        // art: average response time (in milliseconds)
        buf.append("<").append(tag).append(" sample-seconds=\"").append(mSecsPerSample).append("\"")
            .append(" active-requests=\"").append(load.mActiveRequestCount).append("\" >\n");

        buf.append("<one")
            .append(" requests=\"").append(load.m01Requests).append("\"")
            .append(" requests-per-second=\"").append(mFormat.format(load.m01RequestAvg)).append("\"")
            .append(" avg-response-time=\"").append(mFormat.format(load.m01AvgResTime)).append("\"")
            .append(" max-response-time=\"").append(mFormat.format(load.m01MaxResTime)).append("\"")
            .append(" />\n");

        buf.append("<five")
            .append(" requests=\"").append(load.m05Requests).append("\"")
            .append(" requests-per-second=\"").append(mFormat.format(load.m05RequestAvg)).append("\"")
            .append(" avg-response-time=\"").append(mFormat.format(load.m05AvgResTime)).append("\"")
            .append(" max-response-time=\"").append(mFormat.format(load.m05MaxResTime)).append("\"")
            .append(" />\n");

        buf.append("<fifteen")
            .append(" requests=\"").append(load.m15Requests).append("\"")
            .append(" requests-per-second=\"").append(mFormat.format(load.m15RequestAvg)).append("\"")
            .append(" avg-response-time=\"").append(mFormat.format(load.m15AvgResTime)).append("\"")
            .append(" max-response-time=\"").append(mFormat.format(load.m15MaxResTime)).append("\"")
            .append(" />\n");

        buf.append("<total")
            .append(" requests=\"").append(load.mTotalRequestCount).append("\"")
            .append(" responses=\"").append(load.mTotalResponseCount).append("\"")
            .append(" />\n");

        // sample information
        // req: total requests for sample
        // time: sum of requests times during sample (in milliseconds)
        if (verbose) {
            buf.append("<samples>\n");
            int p = mPtr+1;
            int n = 0;
            while ( n++ < m15 ) {
                p %= m15;
                buf.append("<interval")
                    .append(" n=\"").append(n).append("\"")
                    .append(" requests=\"").append(mRequestCount[p]).append("\"")
                    .append(" responses=\"").append(mResponseCount[p]).append("\"")
                    .append(" response-times-sum=\"").append(mResponseTimes[p]).append("\"")
                    .append(" />");
                p++;
            }
            buf.append("</samples>\n");
        }

        buf.append("</").append(tag).append(">\n");

        return buf.toString();
    }


    /**
     * See toXML().
     */
    public String toString() 
    {
        return toXML("load");
    }
}
