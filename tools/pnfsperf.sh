#!/bin/sh
#
#         needful things
#
#########################################################
#
#   try to find a pnfs mountpoint ( take the first )
#
autodetectPnfs() {
  df -k | awk '{ if( NR > 1 )print $NF }' 2>/dev/null | while read mounted
  do
     if [ ! -f "$mounted/.(const)(magic)" ] ; then continue ; fi
     echo "$mounted"
     exit 44
  done
  if [ $? -eq 44 ] ; then exit 0 ; else exit 1 ; fi
}
#########################################################
#
#   find the active databases and produce a list.
#
autodetectDatabases() {
  n=0
  xsum=""
  while : ; do
    db=`cat "$1/.(get)(database)($n)" 2>/dev/null`
    if [ $? -ne 0 ] ; then
       break
    else
       tp=`echo $db | awk -F: '{ print $3 }'`
       en=`echo $db | awk -F: '{ print $4 }'`
       if [ \( "$tp" = "r" \) -a \( "$en" = "enabled" \) ] ; then
         xsum="$xsum $n"
         printf "+" 1>&2
       else
         printf "-" 1>&2
       fi
    fi
    n=`expr $n + 1`
  done
  echo "" 1>&2
  echo $xsum
}
#########################################################
#
#    try to determine the local nfs cache time
#
autodetectUpdate() {
 
  tmm=`cat "$1/.(get)(counters)(0)" | awk -F= '/time/{print $2}'`
  c=1
  while : ; do
     tmx=`cat "$1/.(get)(counters)(0)" | awk -F= '/time/{print $2}'`
     if [ "$tmx" != "$tmm" ] ; then 
       break
     fi
     sleep 1
     printf "." 1>&2
     c=`expr $c + 1`
  done
  echo "$c"
  return 0
}
#####################################
#
#   sum over all transaction
#
checkDb() {
   awk -F= '{
          if( NR == 1 )nm=$1 
          if( NR == 2 )tm=$2
          if( NR > 2 )x+=$2
        }END{
          printf "%s %d %d\n",nm,x,tm
        }' $1
   return 0

}
#
########################################################
#
#                 MAIN part
#
printf "Detecting pnfs FileSystem : "
pnfsFs=`autodetectPnfs`
if [ $? -ne 0 ] ; then 
   echo "!!! No pnfs filesystem found"  1>&2
   exit 3
else
   echo "$pnfsFs"
fi
#
echo "Detecting active databases (may take awhile)"
sum=`autodetectDatabases $pnfsFs`
#
printf  "Detecting update time : "
uptx=`autodetectUpdate $pnfsFs`
upt=`expr \( $uptx / 10 + 1 \) \* 10`
echo " $uptx -> $upt seconds"
#
inter=10
while : ; do
   echo "start"
   for c in $sum ;do
     checkDb "$pnfsFs/.(get)(counters)($c)"
   done
   sleep $inter 
done | \
awk  'BEGIN{ sum = 0 ; }
    /start/{ if( ( sum > 0 ) && ( lastSum > 0 ) )
             printf "%d sec %d transactions %f trans/sec\n",\
                    inter,(sum-lastSum),(sum-lastSum)/inter
             lastSum = sum 
             sum=0 }
           { sum+=$2 }' inter=$inter -
