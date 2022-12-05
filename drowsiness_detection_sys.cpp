#include "drowsiness_detection_sys.h"

drowsiness_detection::drowsiness_detection(void)
{
    camera.open(0);
    if (!camera.isOpened())
    {
        cerr << "Error.....Unable to open camera." << endl;
        exit(0);
    }

    camera.set(CAP_PROP_BUFFERSIZE, 0);
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    //String face_cascade = samples::findFile("C:/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml");
    String face_cascade = samples::findFile("C:/opencv/sources/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml");
    if (!face_detect.load(face_cascade))
    {
        cout << "Error loading face cascade" << endl;
        exit(0);
    };

    try
    {
        //dlib::deserialize("./shape_predictor_68_face_landmarks.dat") >> eye_landmarks;
        dlib::deserialize("./eyes_mouth_nose_tip.dat") >> eye_landmarks;
    }
    catch (dlib::serialization_error & e)
    {
        cout << "Error loading shape predictor" << endl << e.what() << endl;
    }
    catch (exception & e)
    {
        cout << e.what() << endl;
    }

    cout << "Starting system...." << endl
        << "Press any key to terminate." << endl;
}

void drowsiness_detection::cam_read_frame(void)
{
    camera.read(frame);
    if (frame.empty())
        cerr << "Empty frame read." << endl;

    //constexpr int64 kTimeoutNs = 1000;
    //std::vector<int> ready_index;
    //if (VideoCapture::waitAny({ camera }, ready_index, kTimeoutNs)) {
    //    // Camera was ready; get image.
    //    camera.retrieve(frame);
    //}
    //else {
    //    // Camera was not ready; do something else.
    //    cerr << "Empty frame read." << endl;
    //}

    frame_clone = frame.clone();

    cvtColor(frame, frame, COLOR_RGB2GRAY);
}

void drowsiness_detection::dds_loop(void)
{
    while (true)
    {
        cam_read_frame();
        process_image();

        if (face_not_detected)
        {
            cout << "Error...Face not detected" << endl;
        }
        else
        {
            cout << "Face detected" << endl;
            detect_event();
        }

        imshow("Camera View", frame_clone);

        if (waitKey(30) >= 0)
            break;
    }
}

void drowsiness_detection::process_image(void)
{
    vector<Rect> eye_cv;
    vector<dlib::rectangle> eye_dlib;
    vector<Point> left_eye;
    vector<Point> right_eye;

    dlib::cv_image<dlib::rgb_pixel> img(frame_clone);

    equalizeHist(frame, frame);                             //Equalize the image for clear face detection
    //face_detect.detectMultiScale(frame, eye_cv);           //Detect face in the gray frame
    face_detect.detectMultiScale(frame_clone, eye_cv);           //Detect face in the gray frame

    /*
     *  Fail safe in case face is not detected.
     */
    if (eye_cv.empty())
    {
        face_not_detected = true;
        return;
    }
    else
    {
        face_not_detected = false;
        for(int h =0; h < eye_cv.size(); h++)
            rectangle(frame_clone, eye_cv[h], (0, 255, 0));
    }

    convert_rect_CV2DLIB(eye_cv, eye_dlib, 0);

    dlib::full_object_detection shape = eye_landmarks(img, eye_dlib[0]);    //Predict the face landmarks in the detected face


    for (int h = 0; h < shape.num_parts()-1; h++)
        line(frame_clone, convert_point_DLIB2CV(shape.part(h)), convert_point_DLIB2CV(shape.part(h+1)), (0, 255, 0));

    ///*
    // *  Loop to extract the eye landmarks (x,y) coordinates
    // */
    //for (unsigned long k = 36; k < 48; k++)
    //{
    //    temp_xy_dlib = shape.part(k);
    //    temp_xy_cv.x = temp_xy_dlib.x();
    //    temp_xy_cv.y = temp_xy_dlib.y();

    //    if (k < 42)
    //    {
    //        right_eye.push_back(temp_xy_cv);
    //    }
    //    else
    //    {
    //        left_eye.push_back(temp_xy_cv);
    //    }
    //}

    calculate_ear2(shape, &earL);

    //calculate_ear(left_eye, &earL);
    //calculate_ear(right_eye, &earR);

    //ear = (earL + earR) / 2;
}

void drowsiness_detection::detect_event(void)
{
    if (ear < EAR_TH)
    {
        counter++;
        if (counter > COUNT_TH)
        {
            eventcount++;
            cout << "Event triggered. Event Count : " << eventcount << endl;
        }
    }
    else
    {
        counter = 0;
    }
}