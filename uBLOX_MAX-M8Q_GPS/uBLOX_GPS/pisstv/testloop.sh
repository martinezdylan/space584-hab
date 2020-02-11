#!/bin/bash

function say {
  echo $1
  ./dorji freq 144.5
  ./dorji tx
  echo $1 | festival --tts
  ./dorji rx
}

CALLSIGN="4X6UB"

echo "boot up"
echo `date` >> /home/pi/build/flight.log
echo "starting script" >> /home/pi/build/flight.log
echo callsign $CALLSIGN
./dorji init
say "$(./phonetify $CALLSIGN)"
say "starting up balloon mission 4"

echo "23" >/sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio23/direction

COUNT=0
echo "lets go"
while :
do 
  echo "saving data"
  if [ -c /dev/video0 ]; then 
    fswebcam photos/side-`date +%Y_%m_%d_%H:%M:%S`.jpg
  fi
  raspistill -o photos/down-`date +%Y_%m_%d_%H:%M:%S`.jpg
  cat `date` >> /home/pi/build/flight.log
  cat /home/pi/build/gps.json >> /home/pi/build/flight.log
  echo "sending aprs"
  echo "1" > /sys/class/gpio/gpio23/value
  ./piaprs $CALLSIGN -f /home/pi/build/gps.json
  ./dorji freq 144.8
  ./dorji tx
  aplay aprs.wav
  ./dorji rx
  echo "0" > /sys/class/gpio/gpio23/value
  ((COUNT++))
  echo "count = $COUNT"
  if ! (( $COUNT % 5)); then
    echo "sending image"
    echo "{SSTV: $COUNT}" >> /home/pi/build/flight.log
    ./go.sh $COUNT >> /home/pi/build/sstv.log
  fi
  if [ $COUNT -eq 10 ]
  then
    let COUNT=0
  fi
  sleep 60
done
