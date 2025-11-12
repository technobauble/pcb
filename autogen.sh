#! /bin/sh
#
# Run the various GNU autotools to bootstrap the build
# system.  Should only need to be done once.

# for now avoid using bash as not everyone has that installed
CONFIG_SHELL=/bin/sh
export CONFIG_SHELL

############################################################################
#
# Internationalization (i18n) - DISABLED
#
# NOTE: i18n support (gettext/intltool) has been temporarily disabled
# due to compatibility issues with modern tooling. This can be re-enabled
# later once the core build is stable.
#
# To re-enable:
# 1. Uncomment the autopoint and intltoolize sections below
# 2. Uncomment AM_GNU_GETTEXT and IT_PROG_INTLTOOL in configure.ac
# 3. Re-add "po" to SUBDIRS in Makefile.am
#

echo "Skipping internationalization setup (currently disabled)"

# Disabled autopoint (gettext) section
# Disabled intltoolize (intltool) section

############################################################################
#
# automake/autoconf stuff
#

echo "Running aclocal..."
aclocal -I m4 $ACLOCAL_FLAGS || exit 1
echo "Done with aclocal"

echo "Running autoheader..."
autoheader || exit 1
echo "Done with autoheader"

echo "Running automake..."
automake -a -c --foreign || exit 1
echo "Done with automake"

echo "Running autoconf..."
autoconf || exit 1
echo "Done with autoconf"

############################################################################
#
# finish up
#

echo "You must now run configure"

echo "All done with autogen.sh"

