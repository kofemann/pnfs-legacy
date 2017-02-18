#!/bin/sh
#
setupDirectory=/usr/etc
delay=1
pnfsMountDir=/pnfs-tmp-mount
logfile=/tmp/pnfs-install-`date +"%s"`.log
ourRpc=0
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
  if [ "${ourRpc}" -ne 0 ] ; then
     rpcinfo -d 100003 2 >>${logfile} 2>&1
     rpcinfo -d 100005 1 >>${logfile} 2>&1
  fi
  echo " Cleanup done" >>${logfile}
}
displaimer() {
 echo ""
 echo "   WARNING" 
 echo "   --------"
 echo " Do NOT use this installation script in case your are already running" 
 echo " a pnfs service on this node. This software may destroy existing setups"
 echo " and/or databases"
 echo ""
 while : ; do
    printf " Do you want to continue [y/n] : "
    read yesno rest 
    if [ \( "$yesno" = "y" \) -o \( "$yesno" = "yes" \) ] ; then
       echo ""
       return 0
    elif [ \( "$yesno" = "n" \) -o \( "$yesno" = "no" \) ] ; then
       echo ""
       exit 4
    fi
 done
 echo ""
 return 0
}
problem() {
   echo $2 >&2
   echo ""
   cleanUp
   exit $1
}
awkForDirectory() {
   while  : ; do
     if [ $# -eq 1 ] ; then
        printf "  $1 : "
        defanswer=""
     else
        printf "  $1 [$2] : "
        defanswer=$2
     fi
     read answer rest
       if [  -z "${answer}" ] ; then
           if [ -z "${defanswer}" ] ; then
             continue ;
           else
             answer=${defanswer}
             break
           fi
       else
           if [  -d ${answer} ] ; then
              break 
           else
              printf "  The directory '${answer}' doesn't exit\n"
           fi
       fi 
   done
   return 0
}
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
echo ""
echo "  Pnfs Simple Installation"
echo " --------------------------"
echo ""
displaimer
echo " The log file of this session is : ${logfile}"
echo ""
#
# check for other nfs'es
#
printf " Checking nfs servers : " ; sleep ${delay}
rpcinfo -p >/dev/null 2>&1 || problem  5 "'portmap' not running"
rpcinfo -u localhost 100005 >/dev/null 2>&1 && problem 5 "There seems to be already a mountd running"
rpcinfo -u localhost 100003 >/dev/null 2>&1 && problem 5 "There seems to be already a pnfsd running"
echo "Ok"
#
ourRpc=1
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
pnfsDir=`pwd`
pnfsDir=`dirname ${pnfsDir}`
printf "  Pnfs Base Directory : " ; sleep ${delay}
echo ${pnfsDir}
echo " Pnfs Directory detected at : ${pnfsDir}" >>${logfile}
echo ""
echo " Need to ask some questions ... "
echo ""
awkForDirectory "Log Directory" "/var/log"
logDirectory=${answer}
awkForDirectory "Database Directory"
databaseDirectory=${answer}
mkdir -p ${databaseDirectory}/pnfs/info
mkdir -p ${databaseDirectory}/pnfs/trash
mkdir -p ${databaseDirectory}/pnfs/databases
echo ""
echo " Log directory : ${logDirectory}" >>${logfile}
echo " Databases     : ${databaseDirectory}" >>${logfile}
cat >${setupDirectory}/pnfsSetup  <<!
shmkey=1122
shmclients=16
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
pnfsdLevel=3
pmountdLevel=0
dbserverLevel=0
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
UN=`uname -s`
EXTRA_MOUNT_OPTION="-o intr,hard,rw,noac"
#
#  osX(bsd) fix
#
[ "${UN}" = "Darwin" ] && EXTRA_MOUNT_OPTION=${EXTRA_MOUNT_OPTION}",-i,-2" 
#
echo " Mounting pnfs with ${EXTRA_MOUNT_OPTION} " >>${logfile}
printf "             Trying to mount 'pnfs' with ${EXTRA_MOUNT_OPTION} : "
#
mount ${EXTRA_MOUNT_OPTION} localhost:/fs ${pnfsMountDir}>>${logfile} 2>&1 \
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
echo "Ok"
cd ${memory}
umount ${pnfsMountDir} >>${logfile} 2>&1 
echo ""
echo " Remarks :"
echo "    i) You now have to mount pnfs locally"
echo "        e.g. :   mkdir -p /pnfs/fs"
echo "                 mount ${EXTRA_MOUNT_OPTION} localhost:/fs /pnfs/fs"
echo ""
echo "   ii) Any host may now mount this pnfs server"
echo "       !!! Please note that the rem. mountpoints are different"
echo "           for remote and localhost"
echo "         e.g. :" 
echo "         mount ${EXTRA_MOUNT_OPTION}  <thisServerName>:/pnfs /<mountdir>"
echo
echo "  iii) Only localhost can become root within this pnfs instance"
echo ""
exit 0
