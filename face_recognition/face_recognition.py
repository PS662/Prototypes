import argparse
import dlib
import cv2
import numpy as np
import pickle
import os

# Load models
predictor_path = "model/shape_predictor_5_face_landmarks.dat"
face_rec_model_path = "model/dlib_face_recognition_resnet_model_v1.dat"
detector = dlib.get_frontal_face_detector()
sp = dlib.shape_predictor(predictor_path)
facerec = dlib.face_recognition_model_v1(face_rec_model_path)


def GetEmbeddings(img, shape):
    return np.array(facerec.compute_face_descriptor(img, shape))


def SearchTopNFaces(FaceList, _embedding, _nMatches=3, _thresh=0.6):
    distances = []
    for face_data in FaceList:
        face, path, _ = face_data
        dist = np.linalg.norm(face - _embedding)
        if dist < _thresh:
            distances.append((dist, path, face_data))
    distances.sort()
    return distances[:_nMatches]


def load_database():
    try:
        with open('face_database.pkl', 'rb') as f:
            return pickle.load(f)
    except FileNotFoundError:
        return []


def save_database(face_db):
    with open('face_database.pkl', 'wb') as f:
        pickle.dump(face_db, f)


def align_face(img, det):
    try:
        shape = sp(img, det)
        face_chip = dlib.get_face_chip(img, shape)
        return face_chip, shape
    except:
        return img, None  # Return original if alignment fails


def enroll_face(img_path, face_id, skip_center_face, display):
    img = cv2.imread(img_path)
    if img is None:
        print(f"Failed to load image at {img_path}. Check if the file exists and is accessible.")
        return
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    dets = detector(img_rgb, 1)
    face_db = load_database()
    for k, d in enumerate(dets):
        if not skip_center_face:
            img_rgb, shape = align_face(img_rgb, d)
        else:
            shape = sp(img_rgb, d)
        if display:
            cv2.imshow(f"Original Image", img)
            cv2.imshow(f"Enrolling faceImage", img_rgb)
            cv2.waitKey(0)
            cv2.destroyAllWindows()
        face_embedding = GetEmbeddings(img_rgb, shape)
        face_db.append((face_embedding, img_path, img_rgb))
    save_database(face_db)
    print(f"Enrolled face with ID: {face_id}, file path saved: {img_path}")


def recognize_face(img_path, top_n, skip_center_face, display):
    query_img = cv2.imread(img_path)
    if query_img is None:
        print(f"Failed to load image at {img_path}.")
        return
    query_img_rgb = cv2.cvtColor(query_img, cv2.COLOR_BGR2RGB)
    dets = detector(query_img_rgb, 1)
    face_db = load_database()
    for k, d in enumerate(dets):
        if not skip_center_face:
            query_img_rgb, _ = align_face(query_img_rgb, d)
        shape = sp(query_img_rgb, d)
        face_embedding = GetEmbeddings(query_img_rgb, shape)
        matches = SearchTopNFaces(face_db, face_embedding, _nMatches=top_n)
        if display:
            # Convert back to BGR for display
            query_img_bgr = cv2.cvtColor(query_img_rgb, cv2.COLOR_RGB2BGR)
            cv2.imshow("Query Image", query_img_bgr)
        print("Top Matched Images:")
        for dist, _, (matched_embedding, matched_path, matched_face) in matches:
            print(matched_path)
            if display:
                matched_img = cv2.imread(matched_path)
                if matched_img is not None:
                    # Convert the matched face image back to BGR before displaying
                    matched_face_bgr = cv2.cvtColor(matched_face, cv2.COLOR_RGB2BGR)
                    cv2.imshow(f"Matched Image - {matched_path}", matched_face_bgr)
        if display:
            cv2.waitKey(0)
            cv2.destroyAllWindows()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Face Recognition System")
    parser.add_argument("--enroll", help="Path to the image for enrolling a face")
    parser.add_argument("--id", help="ID to enroll the face under")
    parser.add_argument("--recognize", help="Path to the image for recognizing faces")
    parser.add_argument("--top_n", type=int, default=3, help="Number of top matches to return")
    parser.add_argument("--skip-center-face", action="store_true", default=False, help="Skip aligning faces before processing")
    parser.add_argument("--display", action="store_true", default=False, help="Display images using OpenCV")
    args = parser.parse_args()

    if args.enroll and args.id:
        enroll_face(args.enroll, args.id, args.skip_center_face, args.display)
    elif args.recognize:
        recognize_face(args.recognize, args.top_n, args.skip_center_face, args.display)