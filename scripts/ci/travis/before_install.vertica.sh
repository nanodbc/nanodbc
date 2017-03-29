#!/bin/bash -ue

if [[ "$TRAVIS_OS_NAME" == "linux" && "$DB" == "vertica" ]]; then
    sudo wget -nv https://s3.amazonaws.com/circle-support-bucket/vertica_7.0.1-0_amd64.deb
    sudo dpkg -i vertica_7.0.1-0_amd64.deb
    sudo /opt/vertica/sbin/install_vertica --failure-threshold HALT \
                                           --accept-eula \
                                           --dba-user-password dbadmin \
                                           --license CE \
                                           -s localhost
    sudo -u dbadmin /opt/vertica/bin/adminTools -t create_db \
                                                --database ci \
                                                -s localhost \
                                                -p dbadmin
    sudo -u dbadmin openssl req -x509 \
                                -nodes \
                                -days 365 \
                                -newkey rsa:2048 \
                                -keyout /home/dbadmin/ci/v_ci_node0001_catalog/server.key \
                                -out /home/dbadmin/ci/v_ci_node0001_catalog/server.crt \
                                -batch
    sudo -u dbadmin chmod 600 /home/dbadmin/ci/v_ci_node0001_catalog/server.key
    sudo -u dbadmin chmod 600 /home/dbadmin/ci/v_ci_node0001_catalog/server.crt
    sudo -u dbadmin /opt/vertica/bin/adminTools -t stop_db --database ci -p dbadmin
    sudo -u dbadmin sh -c 'echo "EnableSSL = 1" >/home/dbadmin/ci/v_ci_node0001_catalog/vertica.conf'
    sudo -u dbadmin /opt/vertica/bin/adminTools -t start_db --database ci -p dbadmin
fi
