import dlib
import glob
import cv2
 
predictor_path = "model/shape_predictor_5_face_landmarks.dat"
face_rec_model_path = "model/dlib_face_recognition_resnet_model_v1.dat"
 
detector = dlib.get_frontal_face_detector()
sp = dlib.shape_predictor(predictor_path)
facerec = dlib.face_recognition_model_v1(face_rec_model_path)
 
def GetEmbeddings(img, shape):
	return facerec.compute_face_descriptor(img, shape)
 
def FaceSimilarity(face_):
	if len(face_encodings) == 0:
		return np.empty((0))
	return np.min(np.linalg.norm(face_encodings - face_to_compare, axis=1))
 
def SearchTopNFaces(FaceList, _embedding, _nMatches = 5, _thresh = 0.6):	
	MatchedFaces = []
	EmbeddingFaces = []
	for embedding in FaceList:
		dist = FaceSimilarity(_embedding, embedding)
		if (dist < _thresh):
			EmbeddingFaces.append(id)
 
	sorted_mylist = sorted(((v, i) for i, v in enumerate(mylist)), reverse=True)
	result = []
	id = 0
	for i, (value, index) in enumerate(sorted_mylist):
		if id > _nMatches:
			break
		result.append(index)
		id += 1
 
	return MatchedFaces
 
def main():
	#Load faces to memory from pickle
 
	#Run face detector
 
	#Generate face embedding
 
	#Check for matches
 
	#If no Match then enrol with new id (append to list). Id is considered as index for simplicity
 
	#If Match then return top 5
 
if __name__ == '__main__':
	main()