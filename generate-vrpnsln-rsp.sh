#!/bin/sh
target=
#target=:Rebuild
(
cd $(dirname $0)
(
    # Build these two first, always
    echo vrpn
    echo vrpndll
    find * -name "*.vcproj" | \
        egrep -v 'submodules|quat|vrpn.vcproj|vrpndll.vcproj' | \
        sed -e 's:.*/::' -e 's/.vcproj//' | \
        egrep -v 'daq|irect|gen_rpc|testimager_client|python|java|phantom|tmp'
) | while read proj; do
    echo "/t:$proj$target"
done > vrpnsln.rsp
)
