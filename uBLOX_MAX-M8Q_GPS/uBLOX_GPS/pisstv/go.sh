if (( "$1" == "10" )); then
  if [ -c /dev/video0 ]; then
    fswebcam capture.jpg
  else
  raspistill -o capture.jpg
  fi
else
  raspistill -o capture.jpg
fi
echo "saving"
cp capture.jpg photos/sstv-`date +%Y_%m_%d_%H:%M:%S`.jpg
echo "editing"
mogrify -resize 320x256 -rotate 180 \
        -pointsize 14  -fill yellow -stroke none \
        -draw "text 150,20 'qsl via idoroseman.com'" \
        -annotate +10+185 "@gps.txt" \
        -annotate +10+230 "`date`" \
        -pointsize 24 -fill red -stroke black \
        -draw "text 10,30 '4X6UB'" \
        capture.jpg
composite -gravity SouthEast bmc4.png capture.jpg out.jpg
echo "compiling"
./pisstv out.jpg
#./dorji init
./dorji freq 144.5
./dorji tx
./phonetify 4x6ub | festival --tts
echo "high altitude balloon mission 4" | festival --tts
aplay out.jpg.wav
./dorji rx
rm capture.jpg
rm out.jpg
rm out.jpg.wav
