
/* Result Sets Interface */
#ifndef SQL_CRSR
#  define SQL_CRSR
  struct sql_cursor
  {
    unsigned int curocn;
    void *ptr1;
    void *ptr2;
    unsigned int magic;
  };
  typedef struct sql_cursor sql_cursor;
  typedef struct sql_cursor SQL_CURSOR;
#endif /* SQL_CRSR */

/* Thread Safety */
typedef void * sql_context;
typedef void * SQL_CONTEXT;

/* Object support */
struct sqltvn
{
  unsigned char *tvnvsn; 
  unsigned short tvnvsnl; 
  unsigned char *tvnnm;
  unsigned short tvnnml; 
  unsigned char *tvnsnm;
  unsigned short tvnsnml;
};
typedef struct sqltvn sqltvn;

struct sqladts
{
  unsigned int adtvsn; 
  unsigned short adtmode; 
  unsigned short adtnum;  
  sqltvn adttvn[1];       
};
typedef struct sqladts sqladts;

static struct sqladts sqladt = {
  1,1,0,
};

/* Binding to PL/SQL Records */
struct sqltdss
{
  unsigned int tdsvsn; 
  unsigned short tdsnum; 
  unsigned char *tdsval[1]; 
};
typedef struct sqltdss sqltdss;
static struct sqltdss sqltds =
{
  1,
  0,
};

/* File name & Package Name */
struct sqlcxp
{
  unsigned short fillen;
           char  filnam[7];
};
static const struct sqlcxp sqlfpn =
{
    6,
    "ora.pc"
};


static unsigned int sqlctx = 4771;


static struct sqlexd {
   unsigned long  sqlvsn;
   unsigned int   arrsiz;
   unsigned int   iters;
   unsigned int   offset;
   unsigned short selerr;
   unsigned short sqlety;
   unsigned int   occurs;
      const short *cud;
   unsigned char  *sqlest;
      const char  *stmt;
   sqladts *sqladtp;
   sqltdss *sqltdsp;
   unsigned char  **sqphsv;
   unsigned long  *sqphsl;
            int   *sqphss;
            short **sqpind;
            int   *sqpins;
   unsigned long  *sqparm;
   unsigned long  **sqparc;
   unsigned short  *sqpadto;
   unsigned short  *sqptdso;
   unsigned int   sqlcmax;
   unsigned int   sqlcmin;
   unsigned int   sqlcincr;
   unsigned int   sqlctimeout;
   unsigned int   sqlcnowait;
            int   sqfoff;
   unsigned int   sqcmod;
   unsigned int   sqfmod;
   unsigned char  *sqhstv[4];
   unsigned long  sqhstl[4];
            int   sqhsts[4];
            short *sqindv[4];
            int   sqinds[4];
   unsigned long  sqharm[4];
   unsigned long  *sqharc[4];
   unsigned short  sqadto[4];
   unsigned short  sqtdso[4];
} sqlstm = {12,4};

/* SQLLIB Prototypes */
extern void sqlcxt (void **, unsigned int *,
                    struct sqlexd *, const struct sqlcxp *);
extern void sqlcx2t(void **, unsigned int *,
                    struct sqlexd *, const struct sqlcxp *);
extern void sqlbuft(void **, char *);
extern void sqlgs2t(void **, char *);
extern void sqlorat(void **, unsigned int *, void *);

/* Forms Interface */
static const int IAPSUCC = 0;
static const int IAPFAIL = 1403;
static const int IAPFTL  = 535;
extern void sqliem(unsigned char *, signed int *);

typedef struct { unsigned short len; unsigned char arr[1]; } VARCHAR;
typedef struct { unsigned short len; unsigned char arr[1]; } varchar;

#define SQLCODE sqlca.sqlcode

/* cud (compilation unit data) array */
static const short sqlcud0[] =
{12,4130,852,0,0,
5,0,0,0,0,0,27,152,0,0,4,4,0,1,0,1,9,0,0,1,9,0,0,1,10,0,0,1,10,0,0,
36,0,0,2,0,0,30,226,0,0,0,0,0,1,0,
};


#line 1 "ora.pc"

#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/errno.h>

#include "util.h"
#include "dbug.h"
#include "cJSON.h"
#include "dbdriver.h"

/* EXEC SQL INCLUDE sqlda;
 */ 
#line 1 "/usr/lib/oracle/11.2/client64/precomp/public/sqlda.h"
/*
 * $Header: sqlda.h 08-may-2007.05:58:33 ardesai Exp $ sqlda.h 
 */

/***************************************************************
*      The SQLDA descriptor definition                         *
*--------------------------------------------------------------*
*      VAX/3B Version                                          *
*                                                              *
* Copyright (c) 1987, 2007, Oracle. All rights reserved.  *
***************************************************************/


/* NOTES
  **************************************************************
  ***                                                        ***
  *** This file is SOSD.  Porters must change the data types ***
  *** appropriately on their platform.  See notes/pcport.doc ***
  *** for more information.                                  ***
  ***                                                        ***
  **************************************************************
*/

/*  MODIFIED
    ardesai    05/08/07  - Bug[6037057] Undef Y
    apopat     05/08/02  - [2362423] MVS PE to make lines shorter than 79
    apopat     07/31/99 -  [707588] TAB to blanks for OCCS
    lvbcheng   10/27/98 -  change long to int for sqlda
    lvbcheng   08/15/97 -  Move sqlda protos to sqlcpr.h
    lvbcheng   06/25/97 -  Move sqlda protos to this file
    jbasu      01/29/95 -  correct typo
    jbasu      01/27/95 -  correct comment - ub2->sb2
    jbasu      12/12/94 - Bug 217878: note this is an SOSD file
    Morse      12/01/87 - undef L and S for v6 include files
    Richey     07/13/87 - change int defs to long 
    Clare      09/13/84 - Port: Ch types to match SQLLIB structs
    Clare      10/02/86 - Add ifndef SQLDA
*/

#ifndef SQLDA_
#define SQLDA_ 1
 
#ifdef T
# undef T
#endif
#ifdef F
# undef F
#endif

#ifdef S
# undef S
#endif
#ifdef L
# undef L
#endif

#ifdef Y
 # undef Y
#endif
 
struct SQLDA {
  /* ub4    */ int        N; /* Descriptor size in number of entries        */
  /* text** */ char     **V; /* Ptr to Arr of addresses of main variables   */
  /* ub4*   */ int       *L; /* Ptr to Arr of lengths of buffers            */
  /* sb2*   */ short     *T; /* Ptr to Arr of types of buffers              */
  /* sb2**  */ short    **I; /* Ptr to Arr of addresses of indicator vars   */
  /* sb4    */ int        F; /* Number of variables found by DESCRIBE       */
  /* text** */ char     **S; /* Ptr to Arr of variable name pointers        */
  /* ub2*   */ short     *M; /* Ptr to Arr of max lengths of var. names     */
  /* ub2*   */ short     *C; /* Ptr to Arr of current lengths of var. names */
  /* text** */ char     **X; /* Ptr to Arr of ind. var. name pointers       */
  /* ub2*   */ short     *Y; /* Ptr to Arr of max lengths of ind. var. names*/
  /* ub2*   */ short     *Z; /* Ptr to Arr of cur lengths of ind. var. names*/
  };
 
typedef struct SQLDA SQLDA;
 
#endif

/* ----------------- */
/* defines for sqlda */
/* ----------------- */

#define SQLSQLDAAlloc(arg1, arg2, arg3, arg4) sqlaldt(arg1, arg2, arg3, arg4) 

#define SQLSQLDAFree(arg1, arg2) sqlclut(arg1, arg2) 



/* EXEC SQL INCLUDE sqlca;
 */ 
#line 1 "/usr/lib/oracle/11.2/client64/precomp/public/sqlca.h"
/**
 * $Header: sqlca.h 24-apr-2003.12:50:58 mkandarp Exp $ sqlca.h 
 */

/* Copyright (c) 1985, 2003, Oracle Corporation.  All rights reserved.  */
 
/*
NAME
  SQLCA : SQL Communications Area.
FUNCTION
  Contains no code. Oracle fills in the SQLCA with status info
  during the execution of a SQL stmt.
NOTES
  **************************************************************
  ***                                                        ***
  *** This file is SOSD.  Porters must change the data types ***
  *** appropriately on their platform.  See notes/pcport.doc ***
  *** for more information.                                  ***
  ***                                                        ***
  **************************************************************

  If the symbol SQLCA_STORAGE_CLASS is defined, then the SQLCA
  will be defined to have this storage class. For example:
 
    #define SQLCA_STORAGE_CLASS extern
 
  will define the SQLCA as an extern.
 
  If the symbol SQLCA_INIT is defined, then the SQLCA will be
  statically initialized. Although this is not necessary in order
  to use the SQLCA, it is a good pgming practice not to have
  unitialized variables. However, some C compilers/OS's don't
  allow automatic variables to be init'd in this manner. Therefore,
  if you are INCLUDE'ing the SQLCA in a place where it would be
  an automatic AND your C compiler/OS doesn't allow this style
  of initialization, then SQLCA_INIT should be left undefined --
  all others can define SQLCA_INIT if they wish.

  If the symbol SQLCA_NONE is defined, then the SQLCA variable will
  not be defined at all.  The symbol SQLCA_NONE should not be defined
  in source modules that have embedded SQL.  However, source modules
  that have no embedded SQL, but need to manipulate a sqlca struct
  passed in as a parameter, can set the SQLCA_NONE symbol to avoid
  creation of an extraneous sqlca variable.
 
MODIFIED
    lvbcheng   07/31/98 -  long to int
    jbasu      12/12/94 -  Bug 217878: note this is an SOSD file
    losborne   08/11/92 -  No sqlca var if SQLCA_NONE macro set 
  Clare      12/06/84 - Ch SQLCA to not be an extern.
  Clare      10/21/85 - Add initialization.
  Bradbury   01/05/86 - Only initialize when SQLCA_INIT set
  Clare      06/12/86 - Add SQLCA_STORAGE_CLASS option.
*/
 
#ifndef SQLCA
#define SQLCA 1
 
struct   sqlca
         {
         /* ub1 */ char    sqlcaid[8];
         /* b4  */ int     sqlabc;
         /* b4  */ int     sqlcode;
         struct
           {
           /* ub2 */ unsigned short sqlerrml;
           /* ub1 */ char           sqlerrmc[70];
           } sqlerrm;
         /* ub1 */ char    sqlerrp[8];
         /* b4  */ int     sqlerrd[6];
         /* ub1 */ char    sqlwarn[8];
         /* ub1 */ char    sqlext[8];
         };

#ifndef SQLCA_NONE 
#ifdef   SQLCA_STORAGE_CLASS
SQLCA_STORAGE_CLASS struct sqlca sqlca
#else
         struct sqlca sqlca
#endif
 
#ifdef  SQLCA_INIT
         = {
         {'S', 'Q', 'L', 'C', 'A', ' ', ' ', ' '},
         sizeof(struct sqlca),
         0,
         { 0, {0}},
         {'N', 'O', 'T', ' ', 'S', 'E', 'T', ' '},
         {0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0}
         }
#endif
         ;
#endif
 
#endif
 
/* end SQLCA */
/* EXEC SQL INCLUDE sqlcpr;
 */ 
#line 1 "/usr/lib/oracle/11.2/client64/precomp/public/sqlcpr.h"
/*
 * $Header: sqlcpr.h 28-dec-2005.15:32:23 apopat Exp $
 */

/* Copyright (c) 1988, 2005, Oracle. All rights reserved.  */
 
/* NAME
     sqlcpr.h
   FUNCTION
     Contains 'customer' prototypes for the sql* routines generated by the
     precompiler.  
     sqlald, sqlglm, sqlnul and sqlprc are included because, although we don't
     generate calls to them, users may include calls to them.

     'customer' prototypes for dynamic method 4 (i.e., sqlda) reside
     in the sqlda.h public header file.

   NOTES
     There should be no 'modified' notes in this file when it goes to the
     customer.  Remove them as they are put in.  */

#ifndef SQLCA
#  include <sqlca.h>
#endif
#ifndef ORACA
#  include <oraca.h>
#endif

#ifndef SQLPRO
#  define SQLPRO

/* -------------------- */
/* defines for sqlcpr.h */
/* -------------------- */

#define SQL_SUCCESS (sword)0
#define SQL_ERROR (sword)-1

#ifndef SQL_SINGLE_RCTX
#define SQL_SINGLE_RCTX (dvoid *)0
#endif /* SQL_SINGLE_RCTX */

#define SQLErrorGetText(arg1, arg2, arg3, arg4) sqlglmt(arg1, arg2, arg3, arg4)

#define SQLStmtGetText(arg1, arg2, arg3, arg4) sqlglst(arg1, arg2, arg3, arg4)

#define SQLColumnNullCheck(arg1, arg2, arg3, arg4) \
        sqlnult(arg1, arg2, arg3, arg4)

#define SQLNumberPrecV6(arg1, arg2, arg3, arg4) sqlprct(arg1, arg2, arg3, arg4)

#define SQLNumberPrecv7(arg1, arg2, arg3, arg4) sqlpr2t(arg1, arg2, arg3, arg4)

#define SQLVarcharGetLength(arg1, arg2, arg3) sqlvcpt(arg1, arg2, arg3) 

#define SQLGetStride(arg1, arg2, arg3, arg4) sqlstrdt(arg1, arg2, arg3, arg4)

#if defined (__STDC__) || defined (__cplusplus)

#ifdef  __cplusplus
extern "C" {
#endif

/* user-callable functions - non-threaded */
/* -------------------------------------- */

extern void sqlglm( unsigned char*, size_t*, size_t* );
extern void sqlgls( char*, size_t*, size_t* );
extern void sqliem( unsigned char*, signed int* );
extern void sqlnul( unsigned short*, unsigned short*, int* );
extern void sqlprc( unsigned int*, int*, int* );
extern void sqlpr2( unsigned int*, int*, int*);
extern void sqlvcp( unsigned int*, unsigned int* );

/* user-callable functions -- thread safe */
/* -------------------------------------- */

extern void sqlglmt( void*, unsigned char*, size_t*, size_t* ); 

extern void sqlglst( void*, char*, size_t*, size_t* );

extern void sqliemt( void*, unsigned char*, signed int*);

extern void sqlnult( void*, unsigned short*, unsigned short*, int*);

extern void sqlpr2t( void*, unsigned int*, int*, int*);

extern void sqlprct( void*, unsigned int*, int*, int*);

extern void sqlvcpt( void*, unsigned int*, unsigned int*);

extern void SQLExtProcError( void*, char*, size_t );

/* Removed generated Pro* 1.3 entry points */
/* --------------------------------------- */

/* Forms 3.0 compatibility functions */
/* --------------------------------- */

extern int iapprs( unsigned char* );      
extern int iappfo( unsigned char*, unsigned char*, size_t );
extern int exiterr( unsigned char* );

/* user-callable functions - non-threaded */
/* -------------------------------------- */

extern struct SQLDA *sqlald( int, size_t, size_t );
extern void sqlclu( struct SQLDA*);

/* user-callable functions -- thread safe */
/* -------------------------------------- */

extern struct SQLDA *sqlaldt( void*, int, size_t, size_t );

extern void sqlclut( void*, struct SQLDA*);

#ifdef  __cplusplus
}
#endif

#else /* k&r C - not ANSI standard */


/* user-callable functions - non-threaded */
/* -------------------------------------- */

extern void sqlglm( /*_ char*, size_t*, size_t* _*/ );
extern void sqlgls( /*_ char*, size_t*, size_t* _*/);
extern void sqliem( /*_ char*, int* _*/ );
extern void sqlnul( /*_ unsigned short*, unsigned short*, int* _*/ );
extern void sqlprc( /*_ unsigned int*, int*, int* _*/ );
extern void sqlpr2( /*_ unsigned int*, int*, int* _*/);
extern void sqlvcp( /*_ unsigned int *, unsigned int * _*/);
extern void sqlstrd(/*_ ub1 *, ub1 *, size_t _*/);  

/* user-callable functions -- thread safe */
/* -------------------------------------- */

extern void sqlglmt(/*_ void*, char*, size_t*, size_t* _*/ ); 

extern void sqlglst(/*_ void*, char*, size_t*, size_t* _*/);

extern void sqliemt(/*_ void*, char*, int* _*/ );

extern void sqlnult(/*_ void*, unsigned short*, unsigned short*, int* _*/ );

extern void sqlprct(/*_ void*, unsigned int*, int*, int* _*/ );

extern void sqlpr2t(/*_ void*, unsigned int*, int*, int* _*/);

extern void sqlvcpt(/*_ void*, unsigned int *, unsigned int * _*/);

extern void sqlstrdt(/*_ void *, ub1 *, ub1 *, size_t _*/);

extern void SQLExtProcError(/*_ void*, char*, size_t _*/);

extern void SQLRowidGet(/*_ void*, OCIRowid **urid _*/);

/* Removed generated Pro* 1.3 entry points */
/* --------------------------------------- */


/* Forms 3.0 compatibility functions */
/* --------------------------------- */

extern int iapprs( /*_ char* _*/ );      
extern int iappfo( /*_ char*, char*, unsigned int _*/ );
extern int exiterr( /*_ char* _*/ );

/* user-callable functions - non-threaded */
/* -------------------------------------- */

extern struct SQLDA *sqlald( /*_ int, size_t, size_t _*/ );
extern void sqlclu( /*_ struct SQLDA* _*/);

/* user-callable functions -- thread safe */
/* -------------------------------------- */

extern struct SQLDA *sqlaldt(/*_ void*, int, size_t, size_t _*/ );
extern void sqlclut(/*_ void*, struct SQLDA* _*/);

#endif /* k&r C - not ANSI standard */

#endif /* SQLPRO */
/* EXEC SQL INCLUDE sqlapr;
 */ 
#line 1 "/usr/lib/oracle/11.2/client64/precomp/public/sqlapr.h"
/*
 * $Header: sqlapr.h 08-may-2002.12:13:40 apopat Exp $ 
 */

/* Copyright (c) 1994, 2002, Oracle Corporation.  All rights reserved.  */
/*
   NAME
     sqlapr.h - SQLlib ANSI PRototypes
   DESCRIPTION
     Defines ANSI prototypes for externalized SQLLIB functions used in 
     conjunction with OCI
   PUBLIC FUNCTION(S)
     sqllda: Get Logon Data Area for connection
     sqlld2: Logon Data area form 2 -- with host name
     sqlcda: Cursor data area from result set cursor
     sqlcur: PCC cursor from cursor data area

   NOTES

   MODIFIED   (MM/DD/YY)
    apopat     05/08/02 - [2362423] MVS PE to make lines shorter than 79
    apopat     09/20/00 - [1356099] Clarification on use of sqllda
    lvbcheng   01/16/97 - Fix for b2
    jbasu      02/08/95 - Add new prototypes for thread entry pts
    jbasu      09/21/94 - Update sqlcda,sqlcur; move sqlrcn,sqlfcn to sqlefn.h
    jbasu      09/13/94 - Move XA prototypes to sqlefn.h
    jbasu      08/16/94 - use OCI cda_def type from ocidfn.h instead of sqlold
    jbasu      08/16/94 - Creation
*/

#ifndef SQLAPR
#  define SQLAPR

#ifndef OCIDFN
#  include <ocidfn.h>
#endif

#define SQLLDAGetCurrent(arg1, arg2) sqlldat(arg1, arg2)

void sqllda( Lda_Def *lda );
   /* LDA version 1.   Creates an lda for use in OCI programs.
   **   Assumes that we are logged on through an embedded SQL statement.
   **   The lda is filled in using the connect information from the most 
   **   recently executed embedded SQL statement.  So the ONLY way to use
   **   sqllda() is to call sqllda() immediately after the 
   **   EXEC SQL CONNECT... statement.
   */

void sqlldat(dvoid *sqluga, Lda_Def *lda );
  /* Thread-safe version of sqllda.  Takes an extra parameter sqluga, which 
   * is the SQLLIB runtime context. 
   */

/* **************************************************************************/
/* ** SQLLD2 ** "sqlld2" is an extended version of                          */
/*              the sqllda function used to pass a connection to OCI.       */
/*              "sqlld2" can be used at any time to set up an "lda" for use */
/*              by OCI.  It does not need to called immediately after a SQL */
/*              statement using the particular connection (typically the    */
/*              CONNECT itself).                                            */
/****************************************************************************/

#define SQLDAGetNamed(arg1, arg2, arg3, arg4) sqlld2t(arg1, arg2, arg3, arg4)

void sqlld2( Lda_Def *lda, text *hname, sb4 *hnamel );
   /* LDa version 2.  Creates an lda for use in OCI programs.  The difference
   **      between sqlld2 and sqllda is that sqlld2 allows specification of
   **      the database host name to determine the connection to use in
   **      establishing the lda. 
   **  hname - buffer containing the "logical" name for this connection.  This
   **            would be the same identifier used in an AT clause in an
   **            embedded SQL statement.  A null ptr indicates the "default"
   **            database used when there is no AT clause in a SQL statement.
   **  hnamel - length of buffer containing hname.
   */

void sqlld2t( dvoid *sqluga, Lda_Def *lda, text *hname, sb4 *hnamel );
   /*  Thread-safe version of sqlld2. Takes an extra parameter sqluga, which 
    *  is the SQLLIB runtime context. 
    */ 

#define SQLCDAFromResultSetCursor(arg1, arg2, arg3, arg4) \
        sqlcdat(arg1, arg2, arg3, arg4) 

void sqlcda( Cda_Def *cda, dvoid *cur, sword *retval );
/* 
 * Name: sqlcda() - SQLlib result set cursor to a Cursor Data Area in OCI
 * Description: Translates a result set cursor in SQLLIB to a cursor data area
 *              in OCI.  Both of these structs should have been previously 
 *              created/allocated.
 * Input   : cur   - pointer to a result set cursor struct allocated thru Pro*
 * Output  : cda   - pointer to destination cursor data area in OCI
 *           retval- Return value: 0 if no error,SQLLIB error number otherwise
 *
 * Notes: 1. In case of an error, the v2_rc and rc fields of the cda are
 *           populated with the negative and positive error codes respectively.
 *        2. The 'rows processed count' in the cda structure is NOT populated
 *           in this routine.  This field is set to the correct value only
 *           AFTER a fetch is done in OCI using the cda. Same
 *           comment applies to other cda fields like the function type.
 * ===========================================================================
 */
void sqlcdat(dvoid *sqluga, Cda_Def *cda, dvoid *cur, sword *retval );
 /* Thread-safe version of sqlcda().  Takes an extra parameter sqluga, which 
  * is the SQLLIB runtime context. 
  */ 

#define SQLCDAToResultSetCursor(SQL_SINGLE_RCTX, arg1, arg2, arg3) \
        sqlcurt(arg1, arg2, arg3, arg4)

void sqlcur( dvoid *cur, Cda_Def *cda, sword *retval );
/*
 * Name: sqlcur() - SQLlib result set CURsor from an oci cursor data area
 * Description: Translates a cursor data area in OCI to a result set cursor
 *              in SQLLIB.  Both of these structs should have been previously
 *              created/allocated.
 * Input   : cda    - pointer to a cursor data area in OCI
 * Output  : cur    - pointer to a result set cursor struct (previously 
 *                    allocated thru PCC)
 *           retval - Return value: 0 if no error, error code otherwise.
 * Notes  : The sqlca structure for PCC is NOT updated by this routine.  This 
 *          structure gets populated (with error code, rows fetched count etc)
 *          only AFTER a database operation is performed through PCC using the
 *          translated cursor.
 *============================================================================
 */
void sqlcurt(dvoid *sqluga, dvoid *cur, Cda_Def *cda, sword *retval );
 /* Thread-safe version of sqlcur().  Takes an extra parameter sqluga, which 
  * is the SQLLIB runtime context. 
  */ 

#endif /* SQLAPR */

#line 21 "ora.pc"

SQLDA *bind_dp;
SQLDA *select_dp;

static void dbSetErrorWithErrno(error_info *error, int errorno)
{
	if(!error) return;

	error->ecode = errorno;
	snprintf(error->errstr, sizeof(error->errstr), "%s", strerror(error->ecode));
}

static void dbSetError(error_info *error, const char *fmt, ...)
{
	if(!error) return;

	error->ecode = errno;
	va_list vp;
	va_start(vp, fmt);
	vsnprintf(error->errstr, sizeof(error->errstr), fmt, vp);
	va_end(vp);
}

static void dbSetORAError(error_info *error)
{
	char stm[512] = "";
	unsigned char message[512] = "";
	size_t sqlfc = 0, stmlen = 512, message_length = 0, buffer_size = 512;

	if(!error) return;

	sqlgls(stm, &stmlen, &sqlfc);
	sqlglm(message, &buffer_size, &message_length);

	rtrim((unsigned char*)message, message_length);
	if(message[strlen((char*)message)-1] == 0x0a)
		message[strlen((char*)message)-1] = 0x0;

	error->ecode = SQLCODE;
	snprintf(error->errstr, sizeof(error->errstr), "%s", message);
}

static int isSelectStatment(char *stmt)
{
	static char select[] = "SELECT";

	if(strncasecmp(select, stmt, strlen(select)))
		return 0;
	return 1;
}

static int allocDescriptors(HSTMT hstmt, int max_col_cnt, int max_vname_len, int max_iname_len)
{
	DBUG_ENTER(__func__);

	if ((bind_dp = SQLSQLDAAlloc(SQL_SINGLE_RCTX, max_col_cnt,
					max_vname_len, max_iname_len)) == (SQLDA *) 0)
	{
		dbSetError(hstmt->error, "Cannot allocate memory for bind descriptor.");
		DBUG_RETURN(DBO_FAIL); 
	}

	if ((select_dp = SQLSQLDAAlloc (SQL_SINGLE_RCTX, max_col_cnt,
					max_vname_len, max_iname_len)) == (SQLDA *) 0)
	{
		dbSetError(hstmt->error, "Cannot allocate memory for select descriptor.");
		DBUG_RETURN(DBO_FAIL); 
	}

	int i;
	select_dp->N = max_col_cnt;
	for(i = 0; i < max_col_cnt; i++)
	{
		bind_dp->I[i]   = (short*) malloc(sizeof(short));
		select_dp->I[i] = (short*) malloc(sizeof(short));
		bind_dp->V[i]   = (char *) malloc(sizeof(char));
		select_dp->V[i] = (char *) malloc(sizeof(char));
	}

	DBUG_RETURN(DBO_SUCC); 
}

/*
static int runNonQueryStatment(DBDriver *driver, char *statment)
{
	EXEC SQL BEGIN DECLARE SECTION;
	varchar caSqlStmt[ORA_MAX_SQL_LEN];
	EXEC SQL END DECLARE SECTION;

	caSqlStmt.len = sprintf((char*)caSqlStmt.arr, "%s", statment);
	EXEC SQL EXECUTE IMMEDIATE :caSqlStmt;
	if(SQLCODE != IAPSUCC)
	{
		dbSetORAError(driver);
		EXEC SQL ROLLBACK WORK;
		return(IAPFAIL);
	}
	else
	{
		EXEC SQL COMMIT WORK;
		if(SQLCODE != IAPSUCC)
		{
			dbSetORAError(driver);
			return(IAPSUCC);
		}
	}

	dbSetErrorWithErrno(driver, 0);
	return(IAPSUCC);
}
*/

static int Proc_db_connect(HDBC hdbc)
{
	DBUG_ENTER(__func__);

	/* EXEC SQL BEGIN DECLARE SECTION; */ 
#line 137 "ora.pc"

	/* varchar     dbpwd[21]; */ 
struct { unsigned short len; unsigned char arr[21]; } dbpwd;
#line 138 "ora.pc"

	/* varchar     dbuserid[21]; */ 
struct { unsigned short len; unsigned char arr[21]; } dbuserid;
#line 139 "ora.pc"

	/* EXEC SQL END DECLARE SECTION; */ 
#line 140 "ora.pc"


	if(hdbc == NULL)
		DBUG_RETURN(RETURN_INVALID);

	DBUG_PRINT(__func__, ("LINE:%d DBOPENDATABASE BEGIN!!!",__LINE__));

	strcpy((char*)dbuserid.arr, hdbc->username);
	dbuserid.len = strlen(hdbc->username);
	strcpy((char*)dbpwd.arr, hdbc->password);
	dbpwd.len = strlen(hdbc->password);

	/* EXEC SQL CONNECT :dbuserid IDENTIFIED BY :dbpwd; */ 
#line 152 "ora.pc"

{
#line 152 "ora.pc"
 struct sqlexd sqlstm;
#line 152 "ora.pc"
 sqlstm.sqlvsn = 12;
#line 152 "ora.pc"
 sqlstm.arrsiz = 4;
#line 152 "ora.pc"
 sqlstm.sqladtp = &sqladt;
#line 152 "ora.pc"
 sqlstm.sqltdsp = &sqltds;
#line 152 "ora.pc"
 sqlstm.iters = (unsigned int  )10;
#line 152 "ora.pc"
 sqlstm.offset = (unsigned int  )5;
#line 152 "ora.pc"
 sqlstm.cud = sqlcud0;
#line 152 "ora.pc"
 sqlstm.sqlest = (unsigned char  *)&sqlca;
#line 152 "ora.pc"
 sqlstm.sqlety = (unsigned short)4352;
#line 152 "ora.pc"
 sqlstm.occurs = (unsigned int  )0;
#line 152 "ora.pc"
 sqlstm.sqhstv[0] = (unsigned char  *)&dbuserid;
#line 152 "ora.pc"
 sqlstm.sqhstl[0] = (unsigned long )23;
#line 152 "ora.pc"
 sqlstm.sqhsts[0] = (         int  )23;
#line 152 "ora.pc"
 sqlstm.sqindv[0] = (         short *)0;
#line 152 "ora.pc"
 sqlstm.sqinds[0] = (         int  )0;
#line 152 "ora.pc"
 sqlstm.sqharm[0] = (unsigned long )0;
#line 152 "ora.pc"
 sqlstm.sqadto[0] = (unsigned short )0;
#line 152 "ora.pc"
 sqlstm.sqtdso[0] = (unsigned short )0;
#line 152 "ora.pc"
 sqlstm.sqhstv[1] = (unsigned char  *)&dbpwd;
#line 152 "ora.pc"
 sqlstm.sqhstl[1] = (unsigned long )23;
#line 152 "ora.pc"
 sqlstm.sqhsts[1] = (         int  )23;
#line 152 "ora.pc"
 sqlstm.sqindv[1] = (         short *)0;
#line 152 "ora.pc"
 sqlstm.sqinds[1] = (         int  )0;
#line 152 "ora.pc"
 sqlstm.sqharm[1] = (unsigned long )0;
#line 152 "ora.pc"
 sqlstm.sqadto[1] = (unsigned short )0;
#line 152 "ora.pc"
 sqlstm.sqtdso[1] = (unsigned short )0;
#line 152 "ora.pc"
 sqlstm.sqphsv = sqlstm.sqhstv;
#line 152 "ora.pc"
 sqlstm.sqphsl = sqlstm.sqhstl;
#line 152 "ora.pc"
 sqlstm.sqphss = sqlstm.sqhsts;
#line 152 "ora.pc"
 sqlstm.sqpind = sqlstm.sqindv;
#line 152 "ora.pc"
 sqlstm.sqpins = sqlstm.sqinds;
#line 152 "ora.pc"
 sqlstm.sqparm = sqlstm.sqharm;
#line 152 "ora.pc"
 sqlstm.sqparc = sqlstm.sqharc;
#line 152 "ora.pc"
 sqlstm.sqpadto = sqlstm.sqadto;
#line 152 "ora.pc"
 sqlstm.sqptdso = sqlstm.sqtdso;
#line 152 "ora.pc"
 sqlstm.sqlcmax = (unsigned int )100;
#line 152 "ora.pc"
 sqlstm.sqlcmin = (unsigned int )2;
#line 152 "ora.pc"
 sqlstm.sqlcincr = (unsigned int )1;
#line 152 "ora.pc"
 sqlstm.sqlctimeout = (unsigned int )0;
#line 152 "ora.pc"
 sqlstm.sqlcnowait = (unsigned int )0;
#line 152 "ora.pc"
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
#line 152 "ora.pc"
}

#line 152 "ora.pc"

	if( SQLCODE != IAPSUCC )
	{
		dbSetORAError(hdbc->error);
		DBUG_PRINT(__func__, ("LINE:%d CONNECT TO DB ERROR & DBOPENDATABASE END!!!",__LINE__));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_PRINT(__func__, ("LINE:%d DB_LOGIN SUCCESSFULLY END!!!",__LINE__));

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBConnectInitialize(HDBC *hdbc)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	*hdbc = malloc(sizeof(connection));
	if(*hdbc == NULL) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	(*hdbc)->error = malloc(sizeof(error_info));
	if((*hdbc)->error == NULL) {
		mFree(hdbc);
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBConnectFinished(HDBC hdbc)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL)
		DBUG_RETURN(RETURN_INVALID);

	mFree(hdbc->error);
	mFree(hdbc);

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBConnect(HDBC hdbc, char *username, char *password, char *database)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL || username == NULL || password == NULL)
		DBUG_RETURN(RETURN_INVALID);
	
	sprintf(hdbc->username, "%s", username);
	sprintf(hdbc->password, "%s", password);
	sprintf(hdbc->database, "%s", database?database:"");

	if(Proc_db_connect(hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT(__func__, ("%s", hdbc->error->errstr));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBDisconnect(HDBC hdbc)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL)
		DBUG_RETURN(RETURN_INVALID);

	/* EXEC SQL COMMIT WORK RELEASE; */ 
#line 226 "ora.pc"

{
#line 226 "ora.pc"
 struct sqlexd sqlstm;
#line 226 "ora.pc"
 sqlstm.sqlvsn = 12;
#line 226 "ora.pc"
 sqlstm.arrsiz = 4;
#line 226 "ora.pc"
 sqlstm.sqladtp = &sqladt;
#line 226 "ora.pc"
 sqlstm.sqltdsp = &sqltds;
#line 226 "ora.pc"
 sqlstm.iters = (unsigned int  )1;
#line 226 "ora.pc"
 sqlstm.offset = (unsigned int  )36;
#line 226 "ora.pc"
 sqlstm.cud = sqlcud0;
#line 226 "ora.pc"
 sqlstm.sqlest = (unsigned char  *)&sqlca;
#line 226 "ora.pc"
 sqlstm.sqlety = (unsigned short)4352;
#line 226 "ora.pc"
 sqlstm.occurs = (unsigned int  )0;
#line 226 "ora.pc"
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
#line 226 "ora.pc"
}

#line 226 "ora.pc"

	if( SQLCODE != IAPSUCC )
	{
		dbSetORAError(hdbc->error);

		DBUG_PRINT(__func__, ("LINE:%d DBCLOSEDATABASE END UNSUCCESSFULLY!!!The uncommitted transaction hasn't committed!!!",__LINE__));
		DBUG_PRINT(__func__, ("%s", hdbc->error->errstr));

		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_PRINT(__func__, ("LINE:%d DBCLOSEDATABASE SUCCESSFULLY END!!!",__LINE__));

	DBUG_RETURN(RETURN_SUCCESS);
}

#if 0 
static int fetchQueryStatmentResult(DBDriver *driver);
int DBExecute(DBDriver *driver, char *statment)
{
	DBUG_ENTER(__func__);

	int i;
	struct Cprivate *priv = NULL;
	SQLDA *bind_dp = NULL;
	SQLDA *select_dp = NULL;

	EXEC SQL BEGIN DECLARE SECTION;
	char caSqlStmt[ORA_MAX_SQL_LEN];
	EXEC SQL VAR caSqlStmt IS STRING(ORA_MAX_SQL_LEN);
	EXEC SQL END DECLARE SECTION;

	if(driver == NULL)
		DBUG_RETURN(DBO_FAIL);

	priv = (struct Cprivate*)driver->statment;
	sprintf(caSqlStmt, "%s", statment);

	//
	// If is Non query sql statment
	// execute immediate and return
	//
	if( ! isSelectStatment(caSqlStmt))
	{
		driver->sqltype = 0;
		if(IAPSUCC != runNonQueryStatment(driver, statment))
		{
			DBUG_PRINT("runNonQueryStatment", ("%s", driver->errstr));
			DBUG_RETURN(DBO_FAIL);
		}
		//
		// generate non query sql json response
		//
		DBUG_RETURN(DBO_SUCC);
	}

	//
	// Query sql statment shuld be prepare and descript before execute
	//
	if(allocDescriptors(driver, ORA_SQL_MAX_ITEM_NUM, ORA_COLUMN_NAME_LEN, ORA_INDICATE_NAME_LEN) != 0)
	{
		DBUG_PRINT("allocDescriptors", ("%s", driver->errstr));
		DBUG_RETURN(DBO_FAIL);
	}

	bind_dp = priv->bind_dp;
	select_dp = priv->select_dp;

	EXEC SQL PREPARE S FROM :caSqlStmt;
	EXEC SQL DECLARE C CURSOR FOR S;
	EXEC SQL OPEN C USING DESCRIPTOR bind_dp;

	if(SQLCODE != IAPSUCC)
	{
		dbSetORAError(driver);
		DBUG_RETURN(DBO_FAIL);
	}

	if(IAPSUCC != fetchQueryStatmentResult(driver))
	{
		DBUG_PRINT("fetchQueryStatmentResult", ("%s", driver->errstr));
	}

	for (i = 0; i < ORA_SQL_MAX_ITEM_NUM; i++)
	{
		if (bind_dp->V[i] != (char *) 0)
			mFree(bind_dp->V[i]);
		mFree(bind_dp->I[i]);
		if (select_dp->V[i] != (char *) 0)
			mFree(select_dp->V[i]);
		mFree(select_dp->I[i]);
	}
	SQLSQLDAFree(SQL_SINGLE_RCTX, bind_dp);
	SQLSQLDAFree(SQL_SINGLE_RCTX, select_dp);
	EXEC SQL CLOSE C;

	priv->bind_dp = NULL;
	priv->select_dp = NULL;

	if(driver->ecode)
		DBUG_RETURN(DBO_FAIL);
	DBUG_RETURN(DBO_SUCC);
}

static int allocEnoughSpaceForField(db_reader *reader)
{
	int i, null, precision, scale;
	struct Cprivate *priv = (struct Cprivate*)reader->statment;

	SQLDA *sqlda = priv->select_dp;

	for (i = 0; i < sqlda->F; i++)
	{
		SQLColumnNullCheck (SQL_SINGLE_RCTX,
				(unsigned short*)&(sqlda->T[i]), (unsigned short*)&(sqlda->T[i]), &null);
		priv->fields[i].type = sqlda->T[i];

		switch (sqlda->T[i])
		{
			case ORA_SQL_FIELD_TYPE_VCHAR2 :
			case ORA_SQL_FIELD_TYPE_CHAR :
				break;

			case ORA_SQL_FIELD_TYPE_NUMBER :
				SQLNumberPrecV6 (SQL_SINGLE_RCTX,
						(unsigned int*)&(sqlda->L[i]), &precision, &scale);
				// XXX: maybe wrong in this way
				if(precision>0 || scale>0)
					sqlda->L[i] = precision+scale+(scale>0?1:0);
				else
					sqlda->L[i] = 24;
				sqlda->T[i] = 1;
				break;

			case ORA_SQL_FIELD_TYPE_LONG :
				sqlda->L[i] = 240;
				break;

			case ORA_SQL_FIELD_TYPE_ROWID :
				sqlda->L[i] = 18;
				break;

			case ORA_SQL_FIELD_TYPE_DATE :
				sqlda->L[i] = 9;
				sqlda->T[i] = 1;
				break;

			case ORA_SQL_FIELD_TYPE_RAW :
				break;

			case ORA_SQL_FIELD_TYPE_LRAW :
				sqlda->L[i] = 240;
				break;
		}

		if (sqlda->T[i] != ORA_SQL_FIELD_TYPE_LRAW &&
				sqlda->T[i] != ORA_SQL_FIELD_TYPE_NUMBER)
			sqlda->T[i] = ORA_SQL_FIELD_TYPE_VCHAR2;

		if (sqlda->T[i] == ORA_SQL_FIELD_TYPE_NUMBER)
			sqlda->T[i] = scale ? ORA_SQL_FIELD_TYPE_FLOAT : ORA_SQL_FIELD_TYPE_INTEGER;

		priv->fields[i].length = sqlda->L[i];
		priv->fields[i].name_length = sqlda->C[i];
		priv->fields[i].name = malloc(priv->fields[i].name_length + 1);
		sprintf((char*)priv->fields[i].name, "%.*s",
				priv->fields[i].name_length, sqlda->S[i]);

		sqlda->V[i] = realloc(sqlda->V[i], sqlda->L[i] + 1);
	}

	return(IAPSUCC);
}

int fetchQueryStatmentResult(DBDriver *driver)
{
	DBUG_ENTER(__func__);

	int i;
	char value[4000];
	struct Cprivate *priv = driver->statment;

	cJSON *root = cJSON_CreateObject();
	cJSON *field = cJSON_CreateArray();
	cJSON *result = cJSON_CreateArray();

	SQLDA *select_dp = priv->select_dp;
	select_dp->N = ORA_SQL_MAX_ITEM_NUM;

	EXEC SQL DESCRIBE SELECT LIST FOR S INTO select_dp;
	if (select_dp->F < 0)
	{
		dbSetError(driver, "Too many select-list items(%d), maximum is %d\n",
				-(select_dp->F), ORA_SQL_MAX_ITEM_NUM);
		DBUG_RETURN(IAPFAIL);
	}

	select_dp->N = select_dp->F;

	priv->field_count = select_dp->N;
	priv->fields = malloc(sizeof(Cfield)*priv->field_count);
	if(priv->fields == NULL)
	{
		dbSetErrorWithErrno(driver, ENOMEM);
		DBUG_RETURN(IAPFAIL);
	}

	allocEnoughSpaceForField(driver);

	for(i=0; i<priv->field_count; i++)
	{
		cJSON *object = cJSON_CreateObject();
		cJSON_AddItemToObject(object, "name",
				cJSON_CreateString((char*)priv->fields[i].name));
		cJSON_AddItemToObject(object, "type", cJSON_CreateNumber(priv->fields[i].type));
		cJSON_AddItemToObject(object, "capacity",
				cJSON_CreateNumber(priv->fields[i].length));

		cJSON_AddItemToArray(field, object);
	}

	for (;sqlca.sqlerrd[2]<ORA_MAX_ROW_COUNT;)
	{
		EXEC SQL FETCH C USING DESCRIPTOR select_dp;
		if(SQLCODE==1403)
		{
			break;
		}
		else if(SQLCODE<0)
		{
			priv->row_count = 0x0;
			dbSetORAError(driver);
			DBUG_RETURN(IAPFAIL);
		}

		cJSON *row = cJSON_CreateArray();
		for (i = 0; i < select_dp->F; i++)
		{
			memset(value, 0x0, sizeof(value));

			if (*select_dp->I[i] >= 0)
			{
				memcpy(value, select_dp->V[i], select_dp->L[i]);
				rtrim((unsigned char*)value, select_dp->L[i]);

				if(priv->fields[i].type != ORA_SQL_FIELD_TYPE_VCHAR2 &&
						select_dp->T[i]==ORA_SQL_FIELD_TYPE_VCHAR2)
				{
					ltrim((unsigned char*)value, select_dp->L[i]);
					cJSON_AddItemToArray(row, cJSON_CreateNumber(strtoll(value, NULL, 10)));
				}
				else
				{
					cJSON_AddItemToArray(row, cJSON_CreateString(value));
				}
			}
			else
			{
				cJSON_AddItemToArray(row, cJSON_CreateNull());
			}
		}
		cJSON_AddItemToArray(result, row);
	}
	priv->row_count = sqlca.sqlerrd[2];

	cJSON_AddItemToObject(root, "app", cJSON_CreateString("oracle client"));
	cJSON_AddItemToObject(root, "type", cJSON_CreateString("response"));
	cJSON_AddItemToObject(root, "sqltype", cJSON_CreateString("select"));
	cJSON_AddItemToObject(root, "eflag",
			driver->ecode?cJSON_CreateFalse():cJSON_CreateTrue());
	cJSON_AddItemToObject(root, "message",
			driver->ecode?cJSON_CreateString(driver->errstr):cJSON_CreateString("OK"));
	cJSON_AddItemToObject(root, "pid", cJSON_CreateNumber(getpid()));
	cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(time(NULL)));
	cJSON_AddItemToObject(root, "fields", cJSON_CreateNumber(priv->field_count));
	cJSON_AddItemToObject(root, "rows", cJSON_CreateNumber(priv->row_count));

	cJSON_AddItemToObject(root, "field", field);
	cJSON_AddItemToObject(root, "result", result);

	driver->json_string = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	DBUG_RETURN(IAPSUCC);
}
#endif 
