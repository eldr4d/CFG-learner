#!/bin/bash
file=$1
list=$(seq 0.019 -0.001 0.001 | tr , .);
echo ${list[@]}
set -- junk $list
shift
for list; do
	./CFG-learner DataSets/ strange.txt $list
done

