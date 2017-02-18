#!/bin/sh
#
PNFS_CONFIG=/opt/pnfs/etc/pnfs_config
PNFS_DB=`cat $PNFS_CONFIG | grep PNFS_DB | awk '{print $3}'`
PNFS_LOG=`cat $PNFS_CONFIG | grep PNFS_LOG | awk '{print $3}'`
PNFS_OVERWRITE=`cat $PNFS_CONFIG | grep PNFS_OVERWRITE | awk '{print $3}'`
if [ -d $PNFS_DB ]; then
if [ \( "$PNFS_OVERWRITE" = "n" \) -o \( "$PNFS_OVERWRITE" = "no" \) ] ; then
   echo "PNFS is already installed and is not supposed to be overwritten - Exit"
   exit 1
elif [ \( "$PNFS_OVERWRITE" = "y" \) -o \( "$PNFS_OVERWRITE" = "yes" \) ] ; then
      echo ""
fi
   rm -rf $PNFS_DB
fi

#PD=`ls /usr | grep pnfs`
#if [ -d $PD ]; then
#   cd $PD; rm -rf *
#fi
setupDirectory=/usr/etc
delay=1
pnfsMountDir=/pnfs-tmp-mount
logfile=/tmp/pnfs-install-`date +"%s"`.log
rm -rf ${logfile}
date >>${logfile}
#
cleanUp() {
  echo " Cleanup starting " >>${logfile}
  echo " Unmounting ${pnfsMountDir}" >>${logfile}
  umount ${pnfsMountDir} >>${logfile} 2>&1
  echo " Stopping pnfs server" >>${logfile}
  ${pnfsBin}/pnfs.server stop >>${logfile} 2>&1
  echo " Removing databases" >>${logfile}
  rm -rf  ${databaseDirectory}/pnfs/info >/dev/null 2>&1
  rm -rf  ${databaseDirectory}/pnfs/trash >/dev/null 2>&1
  rm -rf  ${databaseDirectory}/pnfs/databases >/dev/null 2>&1
  rpcinfo -d 100003 2 >>${logfile} 2>&1
  rpcinfo -d 100005 1 >>${logfile} 2>&1
  echo " Cleanup done" >>${logfile}
}
displaimer() {
 echo ""
 echo ""
 return 0
}
problem() {
   echo $2 >&2
   echo ""
   cleanUp
   exit $1
}
# dCache Server System
# if [ -z $PNFS_DB ]; then
#   PNFS_DB=/opt/pnfsdb
#fi
#if [ -d $PNFS_DB ]; then
#   rm -rf $PNFS_DB
# fi
mkdir $PNFS_DB
sleep 2
#
PATH=/usr/bin:/bin:/usr/sbin:/sbin:$PATH
#
if [ \( $# -ne 0 \) -a \( "$1" = "deinstall" \) ]  ; then
  [ ! -f /usr/etc/pnfsSetup ] && problem 4 "Pnfs not installed"
  . /usr/etc/pnfsSetup
  pnfsBin=${pnfs}/tools
  databaseDirectory=`dirname ${database}`
  databaseDirectory=`dirname ${databaseDirectory}`
  cleanUp
  exit 0
fi
#
# check for user (we need to be root)
#
which id >/dev/null 2>&1 || problem 5 "Please make 'id' available in PATH"
user=`id -u`
[ "${user}" -ne 0 ] && problem  3  "Need to be root"
#
# is rpcinfo in path
#
which rpcinfo >/dev/null 2>/dev/null || problem 5 "Please make 'rcpinfo' available in PATH"
# displaimer
#
# check for other nfs'es
#
printf " Checking nfs servers : " ; sleep ${delay}
rpcinfo -p >/dev/null 2>&1 || problem  5 "'portmap' not running"
rpcinfo -u localhost 100005 >/dev/null 2>&1 && problem 5 "There seems to be already a mountd running"
rpcinfo -u localhost 100003 >/dev/null 2>&1 && problem 5 "There seems to be already a pnfsd running"
echo "Ok"
#
# 
#
printf "      Preparing setup : " ; sleep ${delay}
#
mkdir -p ${setupDirectory}
#
[ -f ${setupDirectory}/pnfsSetup ] && \
   ( mv ${setupDirectory}/pnfsSetup ${setupDirectory}/pnfsSetup`date +"%s"` )
echo "Ok"
#
#
# guess pnfs installation path ( `pwd`/../ ) if it's not defined
if [ x$1 != x ]
then
	pnfsDir=$1
else
	pnfsDir=`pwd`
	pnfsDir=`dirname ${pnfsDir}`
fi
# awkForDirectory "Log Directory" "/var/log"
if [ -z $PNFS_LOG ]; then
logDirectory=/var/log
else
logDirectory=$PNFS_LOG
fi
if [ -d $PNFS_LOG ]; then
   rm -rf /var/log/pnfsd.log
fi
# logDirectory=/var/log
# awkForDirectory "Database Directory"
databaseDirectory=$PNFS_DB
# databaseDirectory=/mirror/pnfsdb
mkdir -p ${databaseDirectory}/pnfs/info
mkdir -p ${databaseDirectory}/pnfs/trash
mkdir -p ${databaseDirectory}/pnfs/trash/2
mkdir -p ${databaseDirectory}/pnfs/trash/3
mkdir -p ${databaseDirectory}/pnfs/databases
echo ""
echo " Log directory : ${logDirectory}" >>${logfile}
echo " Databases     : ${databaseDirectory}" >>${logfile}
cat >${setupDirectory}/pnfsSetup  <<!
shmkey=1122
shmclients=8
shmservers=8
pnfs=${pnfsDir}
environment=/0/root/fs/admin/etc/environment
database=${databaseDirectory}/pnfs/info
trash=${databaseDirectory}/pnfs/trash
pnfscopies=4
#
#   define md2pMOREINFO  (3)
#   define md2pINFO      (4)
#   define md2pMODINFO   (5)
#
pmountdLog=${logDirectory}/pmountd.log
dbserverLog=${logDirectory}/dbserver.log
pnfsdLog=${logDirectory}/pnfsd.log
pnfsdLevel=5
pmountdLevel=5
dbserverLevel=6
hardlinks=on
netmask=32
levelmask=0:-1:-1:-1:-1:-1:-1:-1
!
#
. ${setupDirectory}/pnfsSetup
#
echo " The following pnfsSetup file has been produced" >>${logfile}
echo " -----------------------------------------------------" >>${logfile}
cat ${setupDirectory}/pnfsSetup >>${logfile}
echo " -----------------------------------------------------" >>${logfile}
pnfsBin=${pnfsDir}/tools
#
for c in admin data1 ; do
   printf " Creating database ${c}\n"
   ${pnfsBin}/mdb create ${c} ${databaseDirectory}/pnfs/databases/${c}
   if [ $? -ne 0 ] ; then  problem 3 "" ; fi
done
echo ""
echo  " Starting pnfs server   ... " >>${logfile}
printf " Starting pnfs server   ... "
${pnfsBin}/pnfs.server start >>${logfile} 2>&1 || problem $? " Failed"
echo "Ok"
#
for c in 0 1 ; do
   printf " Trying to talk to dbserver ${c} [${shmkey}] ... " ; sleep ${delay}
   ${pnfsBin}/sclient dummy ${shmkey} ${c} >>${logfile} 2>&1 || \
     problem 4 "Failed"
   echo "Ok"
done
mkdir -p ${pnfsMountDir}
echo " Mounting pnfs" >>${logfile}
printf "             Trying to mount 'pnfs' : "
mount -o udp,intr,hard,rw,noac localhost:/fs ${pnfsMountDir}>>${logfile} 2>&1 \
      || problem $? " Failed"
echo "Ok" 
#
printf "        Correcting pnfs permissions : " ; sleep ${delay}
for c in usr admin admin/etc admin/etc/exports admin/etc/config ; do
   chmod 0755 ${pnfsMountDir}/${c} >>${logfile} 2>&1
done
echo "Ok"
printf " Detecting wormhole target (config) : "
memory=`pwd`
cd ${pnfsMountDir}/admin/etc >>${logfile} 2>&1 || problem 44 "Failed"
pnfsid=`cat ".(id)(config)" 2>>${logfile} || problem 55 "Failed"`
echo ${pnfsid}
#
printf "                  Digging wormholes : "
for c in 0 1 ; do
  sleep ${delay}
  ${pnfsBin}/sclient getroot ${shmkey} ${c} ${pnfsid} >>${logfile} 2>&1 || \
  problem 56 " dig-${c}-Failed"
  printf " dig-${c}-ok"
done
sleep ${delay}
echo " Done"
#
printf "             Creating database link : " ; sleep ${delay}
cd ${pnfsMountDir}/usr >>${logfile} 2>&1 || problem 44 "Failed"
mkdir ".(1)(data)" >>${logfile} 2>&1
echo "Ok"
#
printf " Setting mount permissions to world : " ; sleep ${delay}
cd ${pnfsMountDir}/admin/etc/exports >>${logfile} 2>&1 || problem 44 "Failed"
echo "/pnfs /0/root/fs/usr/data  30 nooptions" >0.0.0.0..0.0.0.0 \
    2>>${logfile} || problem 66 "Failed"
echo "/fs        /0/root/fs           0   nooptions" >>0.0.0.0..0.0.0.0 \
      2>>${logfile} || problem 66 "Failed"
echo "Ok"
cd ${memory}
umount ${pnfsMountDir} >>${logfile} 2>&1 
echo ""
echo " Remarks :"
# echo "    i) You now have to mount pnfs locally"
# echo "        e.g. :   mkdir -p /pnfs/fs"
# echo "                 mount -o intr,rw,noac,hard localhost:/fs /pnfs/fs"
# echo ""
echo "   ii) Any host may now mount this pnfs server"
# echo "       !!! Please note that the remote mountpoints are different"
# echo "           for remote and localhost"
# echo "         e.g. :" 
echo "         mount -o intr,rw,noac,hard  <thisServerName>:/pnfs /<mountdir>"
# echo
# echo "  iii) Only localhost can become root within this pnfs instance"
echo ""
exit 0
