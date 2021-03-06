#!/bin/bash
gssdp-discover -i wlan2 --timeout=3 > log.gssdp

# now filter out the locations
grep "Location:" log.gssdp | grep "http" | awk '{print $2}' | sort | uniq > log.locations

# loop the locations, wget the file into a temp file
while read l
do
  wget $l -O temp.xml -o log.wget0 -T 1 -t 2

  # if this file contains any reference to Philips hue, then report the ip address
  isHue=`grep -e "<friendlyName>Philips hue" temp.xml | sed 's/.*(\(.*\)).*/\1/'`

  if [ -n "${isHue}" ]
  then
    bridgeIp=$isHue
  fi

done < log.locations
rm temp.xml

if [ -n "$bridgeIp" ]
then
  echo "Discovered a hue bridge: $bridgeIp"
else
  echo "Didn't find a hue bridge"
  exit 1
fi


if [ ! -f log.username ]
then
  wget --post-data='{"devicetype":"remotePLCdevice"}' http://${bridgeIp}/api -O log.username -o log.wget1
fi

if [ -n "$(grep "error" log.username)" ]
then
  echo "Error: you might have forgotten to press the link button, see log.usernameError for details"
  mv log.username log.usernameError
  exit 1
fi

username=`sed 's/.*username\":\"\(.*\)\"}}\]/\1/' log.username`
echo "Your username: $username" 

echo "Getting the lights..."
wget http://${bridgeIp}/api/$username/lights -o log.wget2 -O log.lights

echo "Writing 1 light to outputs.cfg..."
echo "out1 PhilipsHueBridgeOutput $bridgeIp $username 1" > outputs.cfg
