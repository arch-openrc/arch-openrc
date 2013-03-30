OpenRC pkgbuilds
=========

####Groups:####

* openrc-base
* openrc-desktop
* openrc-misc
* openrc-mobile
* openrc-net

####Virtual packages:####

* virtual-cron-openrc
* virtual-timed-openrc

####ToDo####

*rename conflicting /etc/conf.d/* *

- gpm-openrc *done*

- lm_sensors-openrc *done*

- rsyslog-openrc *done*

- bind-openrc

- iptables-openrc

- nfs-utils-openrc

- openssh-openrc

- squid-openrc

- subversion-openrc

- ypbind-openrc

- hostapd-openrc

*remove checks after arch removed orphaned files from packages*

gpm-openrc: /etc/conf.d/gpm existiert im Dateisystem
lm_sensors-openrc: /etc/conf.d/sensord existiert im Dateisystem
rsyslog-openrc: /etc/conf.d/rsyslog existiert im Dateisystem

bind-openrc: /etc/conf.d/named existiert im Dateisystem
iptables-openrc: /etc/conf.d/iptables existiert im Dateisystem
nfs-utils-openrc: /etc/conf.d/nfs existiert im Dateisystem
openssh-openrc: /etc/conf.d/sshd existiert im Dateisystem
squid-openrc: /etc/conf.d/squid existiert im Dateisystem
subversion-openrc: /etc/conf.d/svnserve existiert im Dateisystem
ypbind-openrc: /etc/conf.d/ypbind existiert im Dateisystem

hostapd-openrc: /etc/conf.d/hostapd existiert im Dateisystem

/etc/init.d/device-mapper existiert in 'device-mapper-openrc' und 'lvm2-openrc'
/etc/init.d/dmeventd existiert in 'device-mapper-openrc' und 'lvm2-openrc'
