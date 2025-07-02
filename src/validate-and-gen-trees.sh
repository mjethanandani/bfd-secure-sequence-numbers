#!/bin/sh

#
# First check to see if you need to create/update your yang repository of
# all IETF published YANG models.
#
if [ ! -d ../bin/yang-parameters ]; then
   rsync -avz --delete rsync.iana.org::assignments/yang-parameters ../bin/
fi

for i in ../bin/*\@$(date +%Y-%m-%d).yang
do
    name=$(echo $i | cut -f 1-3 -d '.')
    echo "Validating YANG module $name.yang"
    if test "${name#^example}" = "$name"; then
        response=`pyang --ietf --lint --strict --canonical -p ../bin --max-line-length=72 --tree-line-length=69 $name.yang`
    else            
        response=`pyang --ietf --strict --canonical -p ../bin --max-line-length=72 --tree-line-length=69 $name.yang`
    fi
    if [ $? -ne 0 ]; then
        printf "$name.yang failed pyang validation\n"
        printf "$response\n\n"
        echo
	rm yang/*-tree.txt.tmp
        exit 1
    fi
    response=`yanglint -p ../bin $name.yang -i`
    if [ $? -ne 0 ]; then
       printf "$name.yang failed yanglint validation\n"
       printf "$response\n\n"
       echo
       exit 1
   fi
   echo "$name.yang is VALID"
done
