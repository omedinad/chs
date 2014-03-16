/* 
 * File:   chs_debug.h
 * Author: oscar
 *
 * Created on March 16, 2014, 12:32 AM
 */

#ifndef CHS_DEBUG_H
#define	CHS_DEBUG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Flag set by ‘--verbose’. */
int verbose_flag;
void verbose(const char * format, ...) ;


#ifdef	__cplusplus
}
#endif

#endif	/* CHS_DEBUG_H */

