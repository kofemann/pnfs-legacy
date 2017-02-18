#!/bin/sh
#
if [ $# -ne 1 ]
  then
    echo "Usage : ... <tagName>"
    exit 4
  fi
#
tagToFind=$1
#
problem() {
   echo $2 >&2
   exit $1
}
#
thisDirectory=`pwd`
dir=`basename  ${thisDirectory}`
#
cd ..
pnfsid=`cat ".(id)(${dir})" 2>/dev/null`
[ $? -ne 0 ] && problem 4 "Current working directory is not in pnfs"
#
#  get the tag chain entry
#
pnfsid=`cat ".(showid)(${pnfsid})" | awk '/ Tag /{ print $3 }' 2>/dev/null`
[ -z "${pnfsid}" ] && problem 8 "Couldn't get pnfsid of tag chain from ${pnfsid}"
#
echo "Chain starts with : ${pnfsid}"
#
[ "${pnfsid}" = "000000000000000000000000" ] && exit 0
#
#   loop
#
tagid=""
while : ; do
    tagname=`cat ".(showid)(${pnfsid})" 2>/dev/null | awk '/ Name /{ print $3 }' 2>/dev/null`
    [ -z "${tagname}" ] && problem 9 "Can't get tag name (.(showid)(${pnfsid}))"
    nextid=`cat ".(showid)(${pnfsid})" | awk '/ next ID /{ print $4 }' 2>/dev/null`
    echo "              Tag : ${pnfsid} ${tagname}"
    [ "${tagToFind}" = "${tagname}" ] && tagid=${pnfsid}
    [ "${nextid}" = "000000000000000000000000" ] && break 
    pnfsid=${nextid}
done
[ -z "${tagid}" ] && problem 11 "Tag not found in  this directory ${tagToFind}"
echo ""
echo "Tag id for ${tagname} : ${tagid} , trying to repair it ..."
#
[ ! -f /usr/etc/pnfsSetup ]  && problem 5 "This is not the pnfs server host"
#
. /usr/etc/pnfsSetup
#
SCLIENT=${pnfs}/tools/sclient
#
[ ! -x ${SCLIENT} ] && problem 6 "Can't find 'sclient' tool"
#
${SCLIENT} writedata ${shmkey} ${tagid} 0 0 1 >/dev/null 2>&1
[ $? -ne 0 ] && problem 12 "Problem running scient"
#
cd ${dir}
echo "" >".(tag)(${tagname})"
echo ""
echo " Tag ${tagname} is repaired again but doesn't contain any data"
echo ""
echo " Please use "
echo "      echo \"new content\" >\".(tag)($tagname)\""
echo " to fill it again"
echo ""
#
exit 0
