#!/bin/bash
file=$1
list=$(seq 1 -0.01 0.02 | tr , .);
echo ${list[@]}
set -- junk $list
shift
for list; do
	./CFG-learner DataSets/ strange.txt $list
done
list=$(seq 0.02 -0.001 0.001 | tr , .);
echo ${list[@]}
set -- junk $list
shift
for list; do
	./CFG-learner DataSets/ strange.txt $list
done
