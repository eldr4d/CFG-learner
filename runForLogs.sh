#!/bin/bash
list=$(find $1 -type d | sort | awk '$0 !~ last "/" {print last} {last=$0} END {print last}')
cores=$2
set -- junk $list
shift
rm commands.txt
for word; do
	echo "./CFG-learner $word normal" >> commands.txt
	echo "./CFG-learner $word standard" >> commands.txt
	echo "./CFG-learner $word uniform" >> commands.txt
done
echo ${dir[@]}
xargs --arg-file=commands.txt --max-procs=$cores --replace --verbose /bin/sh -c "{}"
