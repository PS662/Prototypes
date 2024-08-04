1 - Install openvino 2021.2 (Any version with DLStreamer should work ideally)
2 - Set up openvino environment
3 - Run	./build.sh
4 - Run ./build/social_distancing_app -i data/pedestrian.mp4 or "path to your video"
5 - Parameters like human height, focal length, distance threshold, model path are configurable from command line. Run ./build/social_distancing_app -h to get detailed info. [I have assumed humand height as 160 cm, distance as 6 feet and focal length as 700]

Few BUGS:
1 - I have not checked the object type for detection network so at the end you can see  the alarm between vehicle and person, but if we use pure person detection model then this would not be the case. [Did not do so as the label could change with different networks].
2 - Have just implemented basic functionality, not very much comments and OOP style. But have made it a bit modular so that it can be refactored very easily in any design.
3 - For starting point I have used the sample code from DLStreamer itself. [Not a bug, but thought should be disclosed].
