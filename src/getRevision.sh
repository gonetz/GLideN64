SCRIPT_DIRECTORY=`dirname $0`
rev=\"`git rev-parse --short HEAD`\"
echo current revision $rev
echo "#define PLUGIN_REVISION $rev" > $SCRIPT_DIRECTORY/Revision.h
