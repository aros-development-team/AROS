/*
  File: WaitableDouble.java

  Originally written by Doug Lea and released into the public domain.
  This may be used for any purposes whatsoever without acknowledgment.
  Thanks for the assistance and support of Sun Microsystems Labs,
  and everyone contributing, testing, and using this code.

  History:
  Date       Who                What
  23Jun1998  dl               Create public version
*/

package EDU.oswego.cs.dl.util.concurrent;

/**
 * A class useful for offloading waiting and signalling operations
 * on single double variables. 
 * <p>
 * <p>[<a href="http://gee.cs.oswego.edu/dl/classes/EDU/oswego/cs/dl/util/concurrent/intro.html"> Introduction to this package. </a>]
 **/

public class WaitableDouble extends SynchronizedDouble {
  /** 
   * Make a new WaitableDouble with the given initial value,
   * and using its own internal lock.
   **/
  public WaitableDouble(double initialValue) { 
    super(initialValue); 
  }

  /** 
   * Make a new WaitableDouble with the given initial value,
   * and using the supplied lock.
   **/
  public WaitableDouble(double initialValue, Object lock) { 
    super(initialValue, lock); 
  }


  public double set(double newValue) { 
    synchronized (lock_) {
      lock_.notifyAll();
      return super.set(newValue);
    }
  }

  public boolean commit(double assumedValue, double newValue) {
    synchronized (lock_) {
      boolean success = super.commit(assumedValue, newValue);
      if (success) lock_.notifyAll();
      return success;
    }
  }


  public double add(double amount) { 
    synchronized (lock_) {
      lock_.notifyAll();
      return super.add(amount);
    }
  }

  public double subtract(double amount) { 
    synchronized (lock_) {
      lock_.notifyAll();
      return super.subtract(amount);
    }
  }

  public double multiply(double factor) { 
    synchronized (lock_) {
      lock_.notifyAll();
      return super.multiply(factor);
    }
  }

  public double divide(double factor) { 
    synchronized (lock_) {
      lock_.notifyAll();
      return super.divide(factor);
    }
  }


  /**
   * Wait until value equals c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/

  public void whenEqual(double c, Runnable action) throws InterruptedException {
    synchronized(lock_) {
      while (!(value_ == c)) lock_.wait();
      if (action != null) action.run();
    }
  }

  /**
   * wait until value not equal to c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/
  public void whenNotEqual(double c, Runnable action) throws InterruptedException {
    synchronized (lock_) {
      while (!(value_ != c)) lock_.wait();
      if (action != null) action.run();
    }
  }

  /**
   * wait until value less than or equal to c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/
  public void whenLessEqual(double c, Runnable action) throws InterruptedException {
    synchronized (lock_) {
      while (!(value_ <= c)) lock_.wait();
      if (action != null) action.run();
    }
  }

  /**
   * wait until value less than c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/
  public void whenLess(double c, Runnable action) throws InterruptedException {
    synchronized (lock_) {
      while (!(value_ < c)) lock_.wait();
      if (action != null) action.run();
    }
  }

  /**
   * wait until value greater than or equal to c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/
  public void whenGreaterEqual(double c, Runnable action) throws InterruptedException {
    synchronized (lock_) {
      while (!(value_ >= c)) lock_.wait();
      if (action != null) action.run();
    }
  }

  /**
   * wait until value greater than c, then run action if nonnull.
   * The action is run with the synchronization lock held.
   **/
  public void whenGreater(double c, Runnable action) throws InterruptedException {
    synchronized (lock_) {
      while (!(value_ > c)) lock_.wait();
      if (action != null) action.run();
    }
  }

}

