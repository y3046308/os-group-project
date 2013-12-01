#!/bin/bash

curdir=`pwd`
alias cdcomp="cd ~/os-group-project/os161-1.99/kern/compile/ASST$1"
alias cdconf="cd ~/os-group-project/os161-1.99/kern/conf"
cdconf
.config ASST$1
cdcomp
bmake depend
bmake
bmake install
