dnl $Id$
dnl config.m4 for extension rp_orm

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(rp_orm, for rp_orm support,
dnl Make sure that the comment is aligned:
dnl [  --with-rp_orm             Include rp_orm support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(rp_orm, whether to enable rp_orm support,
dnl Make sure that the comment is aligned:
[  --enable-rp_orm           Include rp_orm support])

if test "$PHP_RP_ORM" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-rp_orm -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/rp_orm.h"  # you most likely want to change this
  dnl if test -r $PHP_RP_ORM/$SEARCH_FOR; then # path given as parameter
  dnl   RP_ORM_DIR=$PHP_RP_ORM
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for rp_orm files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       RP_ORM_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$RP_ORM_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the rp_orm distribution])
  dnl fi

  dnl # --with-rp_orm -> add include path
  dnl PHP_ADD_INCLUDE($RP_ORM_DIR/include)

  dnl # --with-rp_orm -> check for lib and symbol presence
  dnl LIBNAME=rp_orm # you may want to change this
  dnl LIBSYMBOL=rp_orm # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $RP_ORM_DIR/$PHP_LIBDIR, RP_ORM_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_RP_ORMLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong rp_orm lib version or lib not found])
  dnl ],[
  dnl   -L$RP_ORM_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(RP_ORM_SHARED_LIBADD)

  PHP_NEW_EXTENSION(rp_orm, rp_orm.c, $ext_shared)
fi
