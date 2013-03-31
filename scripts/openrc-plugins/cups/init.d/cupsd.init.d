#!/usr/sbin/runscript

depend() {
	use net dbus avahi-daemon
	before nfs
	after logger
}

start() {
	ebegin "Starting cupsd"
	checkpath -q -d -m 0755 -o root:lp /run/cups
	checkpath -q -d -m 0511 -o daemon:sys /run/cups/certs
	start-stop-daemon --start --quiet --exec /usr/sbin/cupsd
	eend $?
}

stop() {
	ebegin "Stopping cupsd"
	start-stop-daemon --stop --quiet --exec /usr/sbin/cupsd
	eend $?
}
