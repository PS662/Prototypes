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