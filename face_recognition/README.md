# Face Recognition Prototype

## Overview

This prototype uses `dlib` and OpenCV to detect and recognize faces from images, comparing embeddings to identify matches.

## Setup

### Environment Setup

1. **Clone the repository** and navigate to your project directory.

2. **Create and activate a virtual environment**:

   ```bash
   python3 -m venv venv/frec_env
   source venv/frec_env/bin/activate
   ```
   Ensure your project directory is structured as follows:

   ```
   face_recognition/
   │
   ├── model/
   │   ├── shape_predictor_5_face_landmarks.dat
   │   └── dlib_face_recognition_resnet_model_v1.dat
   │
   ├── data/
   │   └── Five_Faces/
   │       ├── gates/
   │       └── ...
   │
   ├── face_recognition.py
   └── requirements.txt
   ```

3. **Install dependencies**:

   ```bash
   pip install -r requirements.txt
   ```

4. **Initialize Intel oneAPI for MKL support** (optional for performance optimization):

   ```bash
   source /opt/intel/oneapi/setvars.sh
   ```

5. Download and set up the required dlib models.

    ```
    mkdir model && cd model
    wget http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2
    wget http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2
    bzip2 -d shape_predictor_5_face_landmarks.dat.bz2
    bzip2 -d dlib_face_recognition_resnet_model_v1.dat.bz2
    cd ..
    ```

### Data

Thanks to the [5 Faces Dataset](https://www.kaggle.com/datasets/anku5hk/5-faces-dataset) from Kaggle for sample.

## Usage

- **Enroll a face**:
  
  ```bash
  python face_recognition.py --enroll <path_to_image> --id "<unique_id>"
  ```

- **Recognize faces**:

  ```bash
  python face_recognition.py --recognize <path_to_image> --top_n <number_of_matches>
  ```

Example commands:

```bash
python face_recognition.py --enroll data/Five_Faces/gates/gates1.jpg --id "bill_gates"
python face_recognition.py --recognize data/Five_Faces/gates/gates100.jpg --top_n 3
```


Approach
 
•	The program runs a face detector on input image. The detected face is then sent to dlib face recognition model. 
•	This model generates a 128 length face embedding. 
•	This face embedding is then compared with face embeddings that are loaded in memory, if a match is found then (distance less than given threshold) the face with best match is returned. 
•	The face is assigned an ID and added to memory as a new face.
•	To compare two embeddings 
 
 
Possible optimizations
 
1.	Using light-weight models instead of DLIB: Models from Intel Openvino can be used which will be better suited for generating face embedding with low memory footprint. The embedding length will change to 256 in this case.
 
2.	Other distance metrics such as cosine distance can be used as well. If the use case is of limited known identities then a classifier can be trained to predict the id.
 
3.	Parallel Search (dividing DB in batches) can be used to solve scalability issues when face database reaches large sizes of order 1 million or 10 million.
 
4.	I have assumed the faces to be rotation corrected, for real world settings faces can be centred using face landmarks. Also frame sampling can be done to choose best face out of N frames (to drop faces which are occlude or have side profiles)

5. No duplicity checks.