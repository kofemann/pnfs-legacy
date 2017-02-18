#!/bin/sh
PNFS_CONFIG=/opt/pnfs/etc/pnfs_config

if [ ! -f ${PNFS_CONFIG} ]
then
	echo "${PNFS_CONFIG}  file is missing."
	echo "Please use ${PNFS_CONFIG}.template to create one"
	exit 1;
fi


PNFS_INSTALL_DIR=`cat $PNFS_CONFIG | grep PNFS_INSTALL_DIR |awk '{print $3}'`
PNFS_ROOT=`cat $PNFS_CONFIG | grep PNFS_ROOT |awk '{print $3}'`
RV=0
$PNFS_INSTALL_DIR/tools/autoinstall-s.sh ${PNFS_INSTALL_DIR}
RV=`echo $?`
if [ $RV -ne 0 ]; then
 exit 0
fi  
sleep 5
# Configuration completed, stop & unmount PNFS
echo "Installation of PNFS completed - stop PNFS"
$PNFS_INSTALL_DIR/tools/pnfs.server stop
mkdir -p $PNFS_ROOT
mkdir -p $PNFS_ROOT/fs
exit 0
