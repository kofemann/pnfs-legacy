#!/bin/sh
#
# Copyrighted as an unpublished work.
# (c) Copyright 1996,1997,1998 DESY Hamburg DMG-Division
# All rights reserved.
#
#
# RESTRICTED RIGHTS
#
# These programs are supplied under a license.  They may be used,
# disclosed, and/or copied only as permitted under such license
# agreement.  Any copy must contain the above copyright notice and
# this restricted rights notice.  Use, copying, and/or disclosure
# of the programs is strictly prohibited unless otherwise provided
# in the license agreement.
#
#
#set -x
if [ $# -eq 0 ] ; then
 echo " USAGE : observer <shellProc> <args ... >"
 exit 3  
fi
procName=`basename $1`
procDir=`dirname $1`
proc=$1
setup=/usr/etc/pnfsSetup
#
if [ ! -f $setup ] ; then exit 1 ; fi
. $setup 1>/dev/null 2>/dev/null
if [ -z "$database" ] ; then exit 3 ; fi
tools=$pnfs/tools
procInfos=$database/scheduler/$procName

activeFlag=${procInfos}ACTIVE
retryCounter=${procInfos}COUNTER
retryLimit=${procInfos}RETRYLIMIT

procOutput=${procInfos}-output
procMail=${procInfos}-mail

sendMail(){

  filename=$1
  mode=$2
  
  if [ "$mode" = "emergency" ] ; then
     mailaddress=pnfs-emergency@scar.desy.de
  else
     mailaddress=pnfs-emergency@scar.desy.de
  fi
  /usr/ucb/Mail -s "${3:-\"Pnfs Scheduler\"}" $mailaddress <$filename
  

}
shift
#
#  chech wether this proc is enabled
#
if [ ! -f ${procInfos}OK ] ; then exit 0 ; fi
#
#  try to get the semaphore
# 
mkdir $activeFlag 1>/dev/null 2>/dev/null
#
#  if it fails, increment the 'didn't get the semaphore counter'
#
if [ $? -ne 0 ] ; then
  touch ${retryCounter}
  counter=`cat $retryCounter`
  if [ -z "$counter" ] ; then 
     counter=1
  else
     counter=`expr $counter + 1`
  fi
  
  echo $counter >$retryCounter
  if [ -f ${retryLimit} ] ; then
     retry=`cat ${retryLimit}`
     if [ "$retry" -eq $counter ] ; then
        echo "" >$procMail
        echo "  The Pnfs Scheduler found a problem running task !!! $procName !!! " >>$procMail 
        echo "  Date       :" `date`  >>$procMail 
        echo "  Path       : $proc "  >>$procMail
        echo "  The task couldn't start for the $retry 'th time " >>$procMail
        echo "  We will continuously retry ( without resending this message) ">>$procMail
        sendMail $procMail emegency "pnfs scheduler : $procName "
     fi
  
  fi
#  echo " Retry counter now : $counter "
  exit 0
fi
#echo " Executing : $proc $*"
#
#
eval $proc $*  >$procOutput 2>&1
problem=$?
if [ $problem -ne 0 ] ; then
   echo "" >$procMail
   echo "  The Pnfs Scheduler found a problem running task !!! $procName !!! " >>$procMail 
   echo "  Date       :" `date`  >>$procMail 
   echo "  Path       : $proc "  >>$procMail 
   echo "  ReturnCode : $problem " >>$procMail 
   echo "  Output and error messages ... " >>$procMail 
   echo "  -------------------- start of messages ---------------------- ">>$procMail 
   cat $procOutput >>$procMail 2>/dev/null
   echo "  -------------------- end of messages   ---------------------- ">>$procMail 
   sendMail $procMail emegency "pnfs scheduler : $procName "
fi
rmdir $activeFlag   >/dev/null 2>&1
rm -f $retryCounter
#rm -f $procOutput
