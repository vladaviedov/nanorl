#!/bin/sh
# usage: repack.sh obj_dir out in_list
obj_dir=$1
out=$2
shift 2

for sublib in $@; do
	name=$(basename $sublib .a)
	ar -t $sublib | sed -e "s~^~$obj_dir/~" | xargs ar rvsc $out
done
