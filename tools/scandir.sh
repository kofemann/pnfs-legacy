#!/bin/sh
#
#set -x
cursor=`cat ".(get)(cursor)" 2>/dev/null`
if [ $? -ne 0 ] ; then
  echo "Sorry, not a pnfs filesystem ... " 1>&2
  exit 5
fi
tmpfile=/tmp/$$-scandir.tmp
#
#
# split the arguments into the options -<key>=<value> and the
# positional arguments.
#
   args=""
   opts=""
   while [ $# -gt 0 ] ; do
     if expr "$1" : "-.*" >/dev/null ; then
        a=`expr "$1" : "-\(.*\)" 2>/dev/null`
        key=`echo "$a" | awk -F= '{print $1}' 2>/dev/null`
        value=`echo "$a" | awk -F= '{print $2 }' 2>/dev/null`
        if [ -z "$value" ] ; then a="${key}=" ; fi
        eval "$a"
        a="export ${key}"
        eval "$a"
        opts="${opts} $1"
     else
        args="${args} $1"
     fi
     shift 1
   done
#
#
cat ".(get)(cursor)" >$tmpfile
#
rootId=`cat ".(get)(cursor)" 2>/dev/null | awk -F= '/dirID/{ print $2 }'`
cat ".(showid)($rootId)" >>$tmpfile
lastRow=`cat ".(showid)($rootId)" | tail -1`
hashA=`echo $lastRow | awk '{ print $1 }'`
hashB=`echo $lastRow | awk '{ print $2 }'`
#
#
printDirData() {
  xpnfsid=$1
  cat ".(showid)($xpnfsid)" >>$tmpfile 2>/dev/null
  if [ $? -ne 0 ] ; then
     echo "Inconsistent @ $xpnfsid" >>$tmpfile
     return 1 
  fi
  cat ".(showid)($xpnfsid)" | \
     awk 'BEGIN{ n=0 ; l=0 }
       { if( $1 == "ID" )p=$3 ; if(n==2){ printf "%s %s %s %d %s\n",p,$1,$2,l,$3  ; l++ } }
       /Directory Entries/{ n+=1 }'
       
  return 0
}
#
cat ".(showid)($hashA)" >>$tmpfile
echo "" >>$tmpfile
cat ".(showid)($hashB)" >>$tmpfile
echo "" >>$tmpfile

cat ".(showid)($hashA)" | \
   awk 'BEGIN{ n=0 }
        { if(n==1)for(i=1;i<=NF;i++)printf "%s\n",$i }
        /Hash Pointer Entries/{ n=1 }' | while read pnfsid ; do
#
#  loop over the hash entries
#
   if [ "$pnfsid" = "000000000000000000000000" ] ; then continue  ; fi
   nextId=$pnfsid
   while [ $nextId != "000000000000000000000000" ] ; do
# 
#       and follow the linked list
#
      printDirData $nextId
  
      nextId=`cat ".(showid)($nextId)" | grep "next ID" | awk '{ print $4 }'`
      if [ $? -ne 0 ] ; then break ; fi
   done

done
cat ".(showid)($hashB)" | \
   awk 'BEGIN{ n=0 }
        { if(n==1)for(i=1;i<=NF;i++)printf "%s\n",$i }
        /Hash Pointer Entries/{ n=1 }' | while read pnfsid ; do
#
#  loop over the hash entries
#
   if [ "$pnfsid" = "000000000000000000000000" ] ; then continue  ; fi
   nextId=$pnfsid
   while [ $nextId != "000000000000000000000000" ] ; do
# 
#       and follow the linked list
#
      printDirData $nextId
  
      nextId=`cat ".(showid)($nextId)" | grep "next ID" | awk '{ print $4 }'`
      if [ $? -ne 0 ] ; then break ; fi
   done

done
#
if [ "$mode" = "full" ] ; then
   cat $tmpfile
fi
rm -f $tmpfile
#
exit 0
