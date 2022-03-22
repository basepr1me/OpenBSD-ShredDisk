# shreddisk utility for OpenBSD

OpenBSD Disk Shredder

The shreddisk utility is an efficient and simple, NIST compliant disk shredder
tool. You decide how many times to write to the disk and whether to write NULL,
0, or random data.

Usage
-----

Clone the utility:

		got clone ssh://git@github.com/basepr1me/OpenBSD-ShredDisk.git /var/www/got/public/shreddisk.git
		cd ~/repos/
		got checkout /var/www/got/public/shreddisk.git

Installation to ${HOME}/bin:

		cd ~/repos/shreddisk
		make && make install

Running the software:

		doas shreddisk

Author
------

[Tracey Emery](https://github.com/basepr1me/)

See the [License](LICENSE.md) file for more information.
