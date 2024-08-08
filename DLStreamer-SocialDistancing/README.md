# Social Distancing Application

## Installation and Setup

1. **Install OpenVINO 2021.2**  
   Any version with DLStreamer should ideally work.

2. **Set up OpenVINO environment**

3. **Build the Application**
   ```bash
   ./build.sh

4. **Run the Application**

    ```
   ./build/social_distancing_app -i data/pedestrian.mp4

    # or 
   
   ./build/social_distancing_app -i "path to your video"
    ```

5. **Configuration Parameters**

    Human height
    Focal length
    Distance threshold
    Model path

These parameters are configurable from the command line. Run the following command to get detailed information:

```
./build/social_distancing_app -h
```

Default assumptions: Human height: 160 cm, Distance threshold: 6 feet, Focal length: 700.


## Known Bugs and Limitations

- Object Type Detection
    - The application does not check the object type for the detection network. As a result, you might see alarms between vehicles and persons. Using a pure person detection model will eliminate this issue. The label might change with different networks, so this was not implemented.

- Basic Functionality Implementation
    - The current implementation is basic, with limited comments and OOP style. However, it is somewhat modular, making it easy to refactor into any design.

- Starting Point
    - The initial code was taken from DLStreamer samples. This is not a bug but is disclosed for transparency.