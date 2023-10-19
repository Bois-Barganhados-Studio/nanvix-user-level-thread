# Copyright(c) 2011-2023 The Maintainers of Nanvix.
# Licensed under the MIT License.

layout split
target remote tcp::1234
set debug-file-directory bin/
#file bin/kernel
add-symbol-file bin/kernel
add-symbol-file bin/ubin/bthd
handle SIGSEGV nostop noprint nopass
set confirm off
focus cmd
set detach-on-fork
#set breakpoint pending on
b btrestorer

define hook-stop
	if $_isvoid ($_exitcode) != 1
		quit
	end

	focus cmd
end
