#!/bin/ksh
#
# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
#

NSCD_INDICATOR=/var/tsol/doors/nscd_per_label
export LANG=C

# The labeled_parseNet function is used by functions in this file
# and by txzonemgr. So changes must be verified in both places.
# The function takes either a zonecfg net or anet resource, passed in
# via $net. It parses the keywords, and sets the following variables:
#
# ipaddr - the IP address, including the cidr suffix if present
# defaultrouter -if present
# physical - the corresponding global zone interface

function labeled_parseNet {
	typeset alpha

	linkname="..."
	physical="..."
	vlan_id="..."
	ipaddr="..."
	defrouter="..."
	shift 1
	while (( $# > 1)) do
		case $1 in
			"lower-link:")
				physical=$2
				;;
			"physical:")
				physical=$2
				;;
			"address:")
				if [ $2 != "..." ]; then
					ipaddr="$2"
				fi
				;;
			"allowed-address:")
				if [ $2 != "..." ]; then
					ipaddr="$2"
				fi
				;;
			"defrouter:")
				defrouter="$2"
				;;
			"linkname:")
				linkname=$2
				;;
			"vlan-id:")
				vlan_id=$2
				;;
		esac
		shift 1
	done
#
#	If address is a hostname, return IP address
#
	alpha=$(echo $ipaddr|grep ^[A-z]|grep -v :)
	if [[ ! -z $alpha ]]; then
		ipaddr=$(getent hosts $ipaddr|nawk '{print $1}')
		if [[ -z $ipaddr ]] ; then
			ipaddr="..."
		fi
	fi
}

# The _getIPconfig function is only used in this file.
# It is used to determine the hostname and IP address
# of the labeled zone. In addition to the variables set
# by labeled_parseNet, it also sets the variable $hostname.

function _getIPconfig {
	typeset hostattr
	typeset cidr

	ipType=$(zonecfg -z $ZONENAME info ip-type|cut -d" " -f2)
	if [ $ipType = exclusive ] ; then
		net=$(zonecfg -z $ZONENAME info anet 2>/dev/null)
	else
		net=$(zonecfg -z $ZONENAME info net 2>/dev/null)
	fi
	if [[ ! -n $net ]]; then
		hostname=$(hostname)
		ipaddr=127.0.0.1
		return 1
	fi
	net=$(echo $net|sed 's/ not specified/: .../g')
	labeled_parseNet $net
	if [ $ipaddr = ... ]; then
		ipaddr=127.0.0.1
		return 1
	fi

	hostattr=$(zonecfg -z $ZONENAME info attr name=hostname 2>/dev/null)
	if [[ -n $hostattr ]]; then
		hostname=$(echo $hostattr|cut -d" " -f7)
		return 0
	else
		cidr=$(echo $ipaddr|grep /|cut -f2 -d/)
		if [[ -n $cidr ]]; then
			# remove the optional cidr suffix
			# which is input to getNetmask()

			ipaddr=$(echo $ipaddr|cut -f1 -d/)
		fi
		getent=$(getent hosts $ipaddr)
		if [ $? = 0 ]; then
			hostname=$(echo $getent|nawk '{print $2}')
		else
			hostname=$ZONENAME
		fi
		return 0
	fi
}

# The labeled_unsharePasswed function removes the passwd and shadow
# lofs mounts that are part of the shared name service configuration.
# This function is also called by txzonemgr so changes must be
# verified in both files.

function labeled_unsharePasswd {
	zonecfg -z $1 remove fs dir=/etc/passwd >/dev/null 2>&1
	zonecfg -z $1 remove fs dir=/etc/shadow >/dev/null 2>&1
}

# The labeled_sharePasswed function sets the passwd and shadow
# lofs mounts that are part of the shared name service configuration.
# This function is also called by txzonemgr so changes must be
# verified in both files.

function labeled_sharePasswd {
	passwd=$(zonecfg -z $1 info|grep /etc/passwd)
	if [ $? -eq 1 ] ; then
		zonecfg -z $1 "add fs; \
		    set special=/etc/passwd; \
		    set dir=/etc/passwd; \
		    set type=lofs; \
		    add options ro; \
		    end" 2>/dev/null
	fi
	shadow=$(zonecfg -z $1 info|grep /etc/shadow)
	if [ $? -eq 1 ] ; then
		zonecfg -z $1 "add fs; \
		    set special=/etc/shadow; \
		    set dir=/etc/shadow; \
		    set type=lofs; \
		    add options ro; \
		    end" 2>/dev/null
	fi
}

#
# The _auto_home function automates NFS sharing of the minimum labeled
# home directory with higher lableled zones. Reading the lowest
# labeled home directory is required by updatehome(1).
#
# An auto_home_<zonename> automap entry is created and stored in
# /var/tsol/doors/automount, which is then readable to all zones.
#
# Although zone administrators can configure their own automount
# entries, it is done automatically here to make things easier.
#
function _auto_home {
	l_zonepath=$(zoneadm -z $ZONENAME list -p|cut -d : -f4)
	typeset ZONE_ETC_DIR=$l_zonepath/root/etc
	typeset ZONE_AUTO_HOME=$ZONE_ETC_DIR/auto_home_$ZONENAME
	typeset TNZONECFG=/etc/security/tsol/tnzonecfg
	typeset AUTOMOUNTDIR=/var/tsol/doors/automount

	if [ -f $NSCD_INDICATOR -a $1 = clone ] ; then
		if [ ! -f $ZONE_AUTO_HOME -a  $LOGNAME != "root" ]; then
			echo "$LOGNAME\tlocalhost:/export/home/$LOGNAME" >> \
			    $ZONE_AUTO_HOME
		fi
	fi
	if [ ! -f $NSCD_INDICATOR -a $1 = install ] ; then
		cat $ZONE_ETC_DIR/auto_master|grep -v /home > /tmp/am
		echo "/home		/etc/auto_homedir" >> /tmp/am
		mv /tmp/am $ZONE_ETC_DIR/auto_master
		chmod +t $ZONE_ETC_DIR/auto_homedir
	fi

	# Find the labeled zone corresponding to the minimum label
	typeset deflabel=$(chk_encodings -a|grep "Default User Sensitivity"|\
	   sed 's/= /=/'|sed 's/"/'''/g|cut -d"=" -f2)
	typeset DEFAULTLABEL=$(atohexlabel "${deflabel}")

	typeset minzone_entry=$(grep ${DEFAULTLABEL} $TNZONECFG)
	if [[ ! -n $minzone_entry ]]; then
		return
	fi
	typeset minzone=$(echo $minzone_entry|cut -d: -f1)
	typeset am=auto_home_$minzone

	if [ $minzone = $ZONENAME ]; then
		# If the current zone has the minimum label,
		# check if its home directories can be shared.

		# An explicit IP address assignment is required
		# for a zone to be a multilevel NFS server.

		_getIPconfig || return

		# Save the automount entry for other zones to import

		if [ ! -d $AUTOMOUNTDIR ] ; then
			mkdir $AUTOMOUNTDIR
		fi
		echo "+$am" > $AUTOMOUNTDIR/$am
		echo "*	${ipaddr}:/export/home/&" \
		    >> $AUTOMOUNTDIR/$am

		# Configure multilevel NFS ports if
		# this hasn't been done already.

		tncfg -z $ZONENAME "add mlp_private=111/tcp;\
		    add mlp_private=111/udp;\
		    add mlp_private=2049/tcp" >/dev/null 2>&1
	else
		# If an automount map exists, then copy it into the higher
		# labeled zone.

		if [ -f $AUTOMOUNTDIR/$am ]; then
			cp $AUTOMOUNTDIR/$am $ZONE_ETC_DIR/$am
			mountpoint="/zone/${minzone}/home"

			# Add map to auto_master if necessary

			entry=$(grep ^$mountpoint $ZONE_ETC_DIR/auto_master)
			if [ $? != 0 ] ;then
				entry="$mountpoint	$am	-nobrowse"
				echo $entry >> $ZONE_ETC_DIR/auto_master
			fi
		else
			rm $ZONE_ETC_DIR/$am 2>/dev/null
		fi
	fi
}


#
# The create_profile_dir() function is used to create
# a directory of custom SC profiles using svcbundle.
# The directory of custom profiles is for a labeled zone
# using properies from the zone's configuration file and
# global zone settings.
#
# $1 specifies how much to configure.
#    Possible values are "system,identity,network,location,users"
#
# $2 specifies an existing directory for the custom SC profiles.
#

function create_profile_dir {
	#
	# Create milestone-config.xml
	#
	svcbundle -o $2/milestone-config.xml \
	    -s bundle-type=profile -s service-name=milestone/config \
	    -s enabled=true -s instance-name=default \
	    -s instance-property=configuration:configure:boolean:true \
	    -s instance-property=configuration:interactive_config:boolean:false \
	    -s instance-property=configuration:config_groups:astring:$1

	#
	# Create system-identity.xml
	#
	_getIPconfig
	svcbundle -o $2/system-identity.xml \
	    -s bundle-type=profile -s service-name=system/identity \
	    -s enabled=true -s instance-name=node \
	    -s instance-property=config:nodename:astring:$hostname

	#
	# Create system-environment.xml
	#
	typeset locale=$(svcprop -p environment/LANG system/environment:init 2>/dev/null)
	if [ $? = 0 -a $locale != '""' ]; then
		svcbundle -o $2/system-environment.xml \
		    -s bundle-type=profile -s service-name=system/environment \
		    -s enabled=true -s instance-name=init \
		    -s instance-property=environment:LANG:astring:$locale
	fi

	#
	# Create system-timezone.xml
	#
	typeset timezone=$(svcprop -p timezone/localtime system/timezone:default)
	svcbundle -o $2/system-timezone.xml \
	    -s bundle-type=profile -s service-name=system/timezone \
	    -s enabled=true -s instance-name=default \
	    -s instance-property=timezone:localtime:astring:$timezone

	#
	# Create system-console-login.xml
	#
	svcbundle -o $2/system-console-login.xml \
	    -s bundle-type=profile -s service-name=system/console-login \
	    -s enabled=true -s instance-name=default \
	    -s instance-property=ttymon:terminal_type:astring:vt100

	#
	# Create system-config-user.xml
	#
	if [ -f $NSCD_INDICATOR  -a $LOGNAME != "root" ]; then
		rootpwd=$(grep "^root:" /etc/shadow|cut -d : -f2)
		userpwd=$(grep "^${LOGNAME}:" /etc/shadow|cut -d : -f2)
		uidopt=""
		gidopt=""
		descropt=""
		shellopt=""
		getent passwd $LOGNAME|while IFS=: \
		    read name pw uid gid descr home shell; do
			uidopt="instance-property=user_account:uid:count:$uid"
			gidopt="instance-property=user_account:gid:count:$gid"
			descopt="instance-property=user_account:description:astring:$descr"
			shellopt="instance-property=user_account:shell:astring:$shell"
		done

		svcbundle -o $2/system-config-user.xml \
		    -s bundle-type=profile -s service-name=system/config-user \
		    -s enabled=true -s instance-name=default \
		    -s instance-property=root_account:password:astring:$rootpwd \
		    -s instance-property=root_account:type:astring:role \
		    -s instance-property=user_account:login:astring:gfaden \
		    -s instance-property=user_account:password:astring:$userpwd \
		    -s instance-property=user_account:roles:astring:root \
		    -s "$uidopt" \
		    -s "$gidopt" \
		    -s "$descopt" \
		    -s "$shellopt"
	fi
}


# The labeled_reconfigure function is used to generate a
# directory of customized SC profiles for a cloned zone, to setup
# some nameservice-specific properties for the user account.
# It is called by the common clone code when the user runs zoneadm
# without specifying a directory of SC profiles for a labeled zone.

# $1 specifies how much to configure.
# possible values are "system,identity,network,location,users"
#
# $2 specifies if the unconfiguration should be destructive
# possible values are --destructive and null
# Returns 0 if successful, and 1 if mount failure

function labeled_reconfigure {
	curlabel=$(tncfg -z $ZONENAME info label 2>/dev/null)
	if [[ -z $curlabel ]] ; then
		return 0
	fi

	if [ -f $NSCD_INDICATOR ] ; then
		labeled_unsharePasswd $ZONENAME
	else
		labeled_sharePasswd $ZONENAME
	fi

	# change "system" to "users" when partial
	# reconfiguration is supported
	l_zonepath=$(zoneadm -z $ZONENAME list -p|cut -d : -f4)
        #zoneadm -z $ZONENAME mount -f
	typeset zonestate=$(zoneadm -z $ZONENAME list -p | cut -d : -f 3)
#	if [ $zonestate != mounted ] ; then
#		gettext "error getting zone $ZONENAME mounted.\n"
#		return 1
#	fi

	_auto_home clone

	typeset configured SCPROFILE ZONE_PROFILE_DIR
	configured=$l_zonepath/root/var/svc/log/milestone-config:default.log

	# A zone can't be unconfigured until it is configured
	# so if the config service has no log file yet we assume
	# if has never been configured, and we can just place
	# the directory of profiles in the site directory.

	if [ ! -f $configured ]; then
		ZONE_PROFILE_DIR="$l_zonepath/root/etc/svc/profile/site"
		SCPROFILEDIR="$ZONE_PROFILE_DIR/tx_profiles"
		rm -rf "$SCPROFILEDIR"
		mkdir "$SCPROFILEDIR"
		create_profile_dir system "$SCPROFILEDIR"
	elif ([ -f $NSCD_INDICATOR ] || _getIPconfig || [ $1 = identity ]); then
		# Always do sysconfig for hostname change ($1 = identity)
		ZONE_PROFILE_DIR="$l_zonepath/lu/system/volatile"
		SCPROFILEDIR="$ZONE_PROFILE_DIR/tx_profiles"
		rm -rf "$SCPROFILEDIR"
		mkdir "$SCPROFILEDIR"
		create_profile_dir $1 "$SCPROFILEDIR"
		typeset SC_CONFIG_BASE=$(basename $SCPROFILEDIR)
		zlogin -S $ZONENAME "export _UNCONFIG_ALT_ROOT=/a; \
		    /usr/sbin/sysconfig configure -g $1 \
		    -c /system/volatile/$SC_CONFIG_BASE $2"
	fi
#	zoneadm -z $ZONENAME unmount
	return $?
}


# The labeled_configure function is used to setup some nameservice-specific
# properties for the user account.
# It is called by pkgcreatezone code after a labeled zone has been installed.

function labeled_configure {
	typeset l_zonepath=$(zoneadm -z $ZONENAME list -p|cut -d : -f4)
	_auto_home install
	if [ -f $NSCD_INDICATOR ] ; then
		labeled_unsharePasswd $ZONENAME
	else
		labeled_sharePasswd $ZONENAME
	fi
}

# The labeled_makeSCprofile() function is used to create
# a directory of customized SC profiles for a new labeled zone.
# It is called by the common pkgcreatezone code when user runs
# zoneadm without specifying a SC profile for a labeled zone.
# The pathname of the new directory of SC profiles is set in
# the variable $labeled_scprofile which is then passed to
# auto-install in the common pkgcreatezone code.

function labeled_makeSCprofile {
	labeled_scprofile=$(mktemp -d /tmp/tx_profiles.XXXXXX)
	if [ $? -ne 0 ]; then
		print "Failed to create directory $labeled_scprofile"
		exit $ZONE_SUBPROC_TRYAGAIN
	fi
	create_profile_dir system $labeled_scprofile
}
