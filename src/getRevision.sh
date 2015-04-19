rev=\"`git rev-parse --short HEAD`\"
echo current revision $rev
echo "#define PLUGIN_REVISION $rev" > Revision.h