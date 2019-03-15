#ifndef COMMON_H_
#define COMMON_H_


#ifdef __cplusplus
  extern "C" {
#endif

  extern char*
  ft_basename( const char*  name );

  /* print a message and exit */
  extern void
  Panic( const char*  fmt,
         ... );

  /*
   * Read the next UTF-8 code from `*pcursor' and
   * returns its value. `end' is the limit of the
   * input string.
   *
   * Return -1 if the end of the input string is
   * reached, or in case of malformed data.
   */
  extern int
  utf8_next( const char**  pcursor,
             const char*   end );

#ifdef __cplusplus
  }
#endif

#endif /* COMMON_H_ */


/* End */
