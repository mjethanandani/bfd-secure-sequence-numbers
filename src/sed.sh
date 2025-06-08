#!/bin/sh

DATE=`date +%Y-%m-%d`

sed_nc_yang_file() {
   # $1 is the src directory
   # $2 is the dest directory
   # $3 is the YANG module
   sed -e"s/YYYY-MM-DD/$DATE/" $1/$3.yang > $2/$3\@$DATE.yang
}

sed_nc_yang_file yang     ../bin      ietf-bfd-met-keyed-isaac-auth
