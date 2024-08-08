#include <algorithm>
#include <dirent.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>

#include "gst/videoanalytics/video_frame.h"
#include <map>
#include <math.h>

using namespace std;

#define UNUSED(x) (void)(x)

const int FOCAL_LENGTH = 400;
const int DISTANCE_THRESH = 180;
const int HUMAN_HEIGHT = 160;

struct CameraConfig
{
    int focal_length;
    int distance_thresh;
    int human_height;
    CameraConfig()
    {
        focal_length = FOCAL_LENGTH;
        distance_thresh = DISTANCE_THRESH;
        human_height = HUMAN_HEIGHT;
    }
};


struct RegionDistance
{
    int x;
    int y;
    int z;
    cv::Rect bbox;
    float violation_distance; // Not used, can be used to display
    RegionDistance()
    {
        x = 0;
        y = 0;
        z = 0;
        violation_distance = -1.0;
        bbox = cv::Rect();        
    }
};

CameraConfig cameraConfig_g;

std::map<int,float> ProcessSocialDistancing(GVA::VideoFrame &video_frame)
{
    std::vector<RegionDistance> AllRegionsV;
    std::map<int, float> SocialDistanceViolations;
	for(GVA::RegionOfInterest &roi: video_frame.regions())
	{
        RegionDistance regionDistance;
       

		auto rect = roi.rect();
		int midX = (rect.x + (rect.x + rect.w)) / 2.0; //todo 2 * rect
		int midY = (rect.y + (rect.y + rect.h)) / 2.0; //todo 2 * rect
		int height =  (rect.y + rect.h) - rect.y;

		regionDistance.z = (cameraConfig_g.human_height * cameraConfig_g.focal_length) /height;

		regionDistance.x = (regionDistance.z * midX) / cameraConfig_g.focal_length;
		regionDistance.y = (regionDistance.z * midY) / cameraConfig_g.focal_length;
        //regionDistance.roi = roi;
        cv::Rect region;
        region.x = rect.x;
		region.y = rect.y;
		region.width = rect.w;
		region.height = rect.h;
        AllRegionsV.emplace_back(regionDistance);
    }

    for (int i = 0; i < AllRegionsV.size(); i++)
    {
        for (int j = 0; j < AllRegionsV.size(); j++)
        {
            if (i < j)
            {
                auto region1 = AllRegionsV[i];
                auto region2 = AllRegionsV[j];
                float eucDist = sqrt(pow((region1.x - region2.x), 2) + pow((region1.y - region2.y),2) + pow((region1.z - region2.z),2));
                if (eucDist < cameraConfig_g.distance_thresh)
                {
                    SocialDistanceViolations.insert({i, eucDist});
                    SocialDistanceViolations.insert({j, eucDist});
                }
            }
        }
    }

	return SocialDistanceViolations;
}

#if 1
bool CheckSocialDistancing(const int &roi_index, std::map<int, float> &SocialDistanceViolations)
{
    if (SocialDistanceViolations.find(roi_index) != SocialDistanceViolations.end())
	    return true;
    return false;
}
#endif

std::vector<std::string> SplitString(const std::string input, char delimiter = ':') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void ExploreDir(std::string search_dir, const std::string &model_name, std::vector<std::string> &result) {
    if (auto dir_handle = opendir(search_dir.c_str())) {
        while (auto file_handle = readdir(dir_handle)) {
            if ((!file_handle->d_name) || (file_handle->d_name[0] == '.'))
                continue;
            if (file_handle->d_type == DT_DIR)
                ExploreDir(search_dir + file_handle->d_name + "/", model_name, result);
            if (file_handle->d_type == DT_REG) {
                std::string name(file_handle->d_name);
                if (name == model_name)
                    result.push_back(search_dir + "/" + name);
            }
        }
        closedir(dir_handle);
    }
}

std::vector<std::string> FindModel(const std::vector<std::string> &search_dirs, const std::string &model_name) {
    std::vector<std::string> result = {};
    for (std::string dir : search_dirs) {
        ExploreDir(dir + "/", model_name, result);
    }
    return result;
}

std::string to_upper_case(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::map<std::string, std::string> FindModels(const std::vector<std::string> &search_dirs,
                                              const std::vector<std::string> &model_names,
                                              const std::string &precision) {
    std::map<std::string, std::string> result;
    for (std::string model_name : model_names) {
        std::vector<std::string> model_paths = FindModel(search_dirs, model_name);
        if (model_paths.empty())
            throw std::runtime_error("Can't find file for model: " + model_name);
        result[model_name] = model_paths.front();
        // The path to the model must contain the precision (/FP32/ or /INT8/)
        for (auto &model_path : model_paths)
            // TODO extract precision from xml file
            if (to_upper_case(model_path).find(to_upper_case(precision)) != std::string::npos) {
                result[model_name] = model_path;
                break;
            }
    }
    return result;
}

const std::string env_models_path =
    std::string() + (getenv("MODELS_PATH") != NULL
                         ? getenv("MODELS_PATH")
                         : getenv("INTEL_CVSDK_DIR") != NULL
                               ? std::string() + getenv("INTEL_CVSDK_DIR") + "/deployment_tools/intel_models" + "/"
                               : "");

const std::vector<std::string> default_detection_model_names = {"face-detection-adas-0001.xml"};

const std::vector<std::string> default_classification_model_names = {
    "facial-landmarks-35-adas-0002.xml", "age-gender-recognition-retail-0013.xml",
    "emotions-recognition-retail-0003.xml", "head-pose-estimation-adas-0001.xml"};

gchar const *detection_model = NULL;
gchar const *classification_models = NULL;

gchar const *input_file = NULL;
gchar const *extension = NULL;
gchar const *device = "CPU";
gchar const *model_precision = "FP32";
gint batch_size = 1;
gdouble threshold = 0.4;
gboolean no_display = FALSE;
// This structure will be used to pass user data (such as memory type) to the
// callback function.
//Option list
static GOptionEntry opt_entries[] = {
    {"input", 'i', 0, G_OPTION_ARG_STRING, &input_file, "Path to input video file", NULL},
    {"extension", 'e', 0, G_OPTION_ARG_STRING, &extension, "Path to custom layers extension library", NULL},
    {"focal length", 'f', 0, G_OPTION_ARG_INT, &cameraConfig_g.focal_length, "Focal Length", NULL},
    {"human height", 'w', 0, G_OPTION_ARG_INT, &cameraConfig_g.human_height, "Human Height", NULL},
    {"distance thresh", 'q', 0, G_OPTION_ARG_INT, &cameraConfig_g.distance_thresh, "Threshold for Social Distancing", NULL},
    {"precision", 'p', 0, G_OPTION_ARG_STRING, &model_precision, "Models precision. Default: FP32", NULL},
    {"device", 'd', 0, G_OPTION_ARG_STRING, &device, "Device to run inference", NULL},
    {"batch", 'b', 0, G_OPTION_ARG_INT, &batch_size, "Batch size", NULL},
    {"threshold", 't', 0, G_OPTION_ARG_DOUBLE, &threshold, "Confidence threshold for detection (0 - 1)", NULL},
    {"no-display", 'n', 0, G_OPTION_ARG_NONE, &no_display, "Run without display", NULL},
    GOptionEntry()};

// This structure will be used to pass user data (such as memory type) to the callback function.
// Printing classification results on a frame
// Gets called to notify about the current blocking type
static GstPadProbeReturn pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    UNUSED(user_data);

    // Create buffer with data from GstPadProbeInfo
    auto buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    // Making a buffer writable can fail (for example if it cannot be copied and is used more than once)
    // buffer = gst_buffer_make_writable(buffer);
    // If pad does not contain data then do nothing

    if (buffer == NULL)
        return GST_PAD_PROBE_OK;

    // Get capabilities describing media types currently configured on pad
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps)
        throw std::runtime_error("Can't get current caps");
    // Construct VideoFrame instance from GstBuffer and GstCaps
    // GVA::VideoFrame controls particular inferenced frame and attached
    // GVA::RegionOfInterest and GVA::Tensor instances
    GVA::VideoFrame video_frame(buffer, caps);
    // Get size of region of interest
    gint width = video_frame.video_info()->width;
    gint height = video_frame.video_info()->height;

    // Map buffer and create OpenCV image
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
        return GST_PAD_PROBE_OK;
    cv::Mat mat(height, width, CV_8UC4, map.data);
    std::map <int, float> SocialDistanceViolations = ProcessSocialDistancing(video_frame);
    // Iterate detected objects and all attributes (tensors)
    int roi_index = 0;
    for (GVA::RegionOfInterest &roi : video_frame.regions()) {
        // Get GstVideoRegionOfInterestMeta from region
        string label;
        float head_angle_r = 0, head_angle_p = 0, head_angle_y = 0;
        auto rect = roi.rect();        
            cv::Scalar color(0,255,0);
            if (CheckSocialDistancing(roi_index, SocialDistanceViolations))
                color = cv::Scalar(0,0,255);
            
				cv::Rect r;
				r.x = rect.x;
				r.y = rect.y;
				r.width = rect.w;
				r.height = rect.h;
				rectangle(mat, r, color);
                roi_index++;                  
	}	
    

    // Release the memory previously mapped with gst_buffer_map
    gst_buffer_unmap(buffer, &map);
    // Unref a GstCaps and and free all its structures and the structures' values
    gst_caps_unref(caps);
    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    return GST_PAD_PROBE_OK;
}

// The entry point for the GVA draw_face_attributes sample application
// Sample recieves video with faces as an argument
// If video file is not passed as an argument obviously, an attempt will be made
// to use camera
int main(int argc, char *argv[]) {
    // Parse arguments
    if (argc < 2)
    {
        std::cout<<"Invalid usage: "<<argv[0]<<" -h for help"<<'\n';
        return -1;
    }

    GOptionContext *context = g_option_context_new("sample");
    g_option_context_add_main_entries(context, opt_entries, "sample");
    g_option_context_add_group(context, gst_init_get_option_group());
    GError *error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("option parsing failed: %s\n", error->message);
        return 1;
    }
    // Construct the pipeline

    // If video file is not passed as an argument, an attempt will be made to use
    // camera
    gchar const *video_source = NULL;
    std::string input_str;
    if (input_file) {
        input_str = (input_file);
        if (input_str.find("/dev/video") != std::string::npos) {
            video_source = "v4l2src device";
        } else if (input_str.find("://") != std::string::npos) {
            video_source = "urisourcebin buffer-size=4096 uri";
        } else {
            video_source = "filesrc location";
        }
    } else {
        input_file = "/dev/video0";
        video_source = "v4l2src device";
    }
    if (env_models_path.empty()) {
        throw std::runtime_error("Enviroment variable MODELS_PATH is not set");
    }
    std::map<std::string, std::string> model_paths;
    std::string classify_str = "";
    if (detection_model == NULL) {
        for (const auto &model_to_path :
             FindModels(SplitString(env_models_path), default_detection_model_names, model_precision))
            model_paths.emplace(model_to_path);
        detection_model = g_strdup(model_paths["face-detection-adas-0001.xml"].c_str());
    }
	detection_model = "model/person-vehicle-bike-detection-crossroad-0078.xml";
    if (classification_models == NULL) {
        for (const auto &model_to_path :
             FindModels(SplitString(env_models_path), default_classification_model_names, model_precision))
            classify_str += "gvaclassify model=" + model_to_path.second + " device=" + device +
                            " batch-size=" + std::to_string(batch_size) + " ! queue ! ";
    }

    gchar const *preprocess_pipeline = "decodebin ! videoconvert n-threads=4 ! videoscale n-threads=4 ";
    gchar const *capfilter = "video/x-raw,format=BGRA";
    gchar const *sink = no_display ? "identity signal-handoffs=false ! fakesink sync=false"
                                   : "fpsdisplaysink video-sink=xvimagesink sync=false";

    // Build the pipeline
    auto launch_str = g_strdup_printf("%s=%s ! %s ! capsfilter caps=\"%s\" ! "
                                      "gvadetect name=gvadetect model=%s device=%s batch-size=%d ! queue ! "                                      
                                      "videoconvert n-threads=4 ! %s",
                                      video_source, input_file, preprocess_pipeline, capfilter, detection_model, device,
                                      batch_size, sink);
    g_print("PIPELINE: %s \n", launch_str);
    GstElement *pipeline = gst_parse_launch(launch_str, NULL);
    g_free(launch_str);
	//launch_str += pipeline_new;

    // set probe callback
    auto gvawatermark = gst_bin_get_by_name(GST_BIN(pipeline), "gvadetect");
    auto pad = gst_element_get_static_pad(gvawatermark, "src");
    // The provided callback 'pad_probe_callback' is called for every state that
    // matches GST_PAD_PROBE_TYPE_BUFFER to probe buffers
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_callback, NULL, NULL);
    gst_object_unref(pad);

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GstBus *bus = gst_element_get_bus(pipeline);

    int ret_code = 0;

    GstMessage *msg = gst_bus_poll(bus, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS), -1);

    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = NULL;
        gchar *dbg_info = NULL;

        gst_message_parse_error(msg, &err, &dbg_info);
        g_printerr("ERROR from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");

        g_error_free(err);
        g_free(dbg_info);
        ret_code = -1;
    }

    if (msg)
        gst_message_unref(msg);

    // Free resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return ret_code;
}