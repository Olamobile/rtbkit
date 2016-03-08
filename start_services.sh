#!/bin/bash
echo "Starting graphite...."
sudo /opt/graphite/bin/carbon-cache.py start
echo "Restarting uwsgi..."
sudo /etc/init.d/uwsgi restart
echo "Restarting nginx..."
sudo /usr/local/nginx/sbin/nginx -s reload
echo "Starting zookeeper..."
~/local/bin/zookeeper/bin/zkServer.sh start
echo "Starting redis..."
redis-server /home/rtbkit/rtbkit_root/rtbkit/rtbkit/sample.redis.conf &

