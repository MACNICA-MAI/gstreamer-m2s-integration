############
#  xhost +
############
GST_PLUGIN_PATH=gstreamer LD_LIBRARY_PATH=library gst-launch-1.0 -v m2svideosrc l1-cpu-num=-1 l2-cpu-num=-1 gpu-num=0 scan=0 p-if-address="192.168.1.23" s-if-address="192.168.2.23" p-dst-address="239.7.20.100" s-dst-address="239.7.21.100" p-src-address="192.168.10.100" s-src-address="192.168.11.100" p-dst-port=50020 s-dst-port=50020 payload-type=96 ! video/x-raw,format=UYVP,width=3840,height=2160,framerate=60000/1001 ! queue ! videoscale ! video/x-raw,width=480,height=270 ! videoconvert ! ximagesink display=:0
