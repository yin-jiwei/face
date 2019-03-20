//
// Created by yjw on 8/22/18.
//

#include <sstream>
#include <opencv2/opencv.hpp>
#include "RESTClient.h"
#include "json.h"
#include "Logger.h"
#include "Base64.h"

using namespace cv;

/* Don't need a namespace table. We put an empty one here to avoid link errors */
extern struct Namespace namespaces[];

bool RESTClient::SendFace(PFaceInfo p_face)
{
    string log_message;
    bool rtn = true;

    if (p_face == nullptr)
        return false;

    unsigned char *jpeg_data = new unsigned char[BMP_SIZE];
    int jpeg_size = 0;
    memset(jpeg_data, 0, BMP_SIZE);

    unsigned char *base64_data = new unsigned char[BMP_SIZE];
    memset(base64_data, 0, BMP_SIZE);

    soap *ctx = soap_new1(SOAP_C_UTFSTRING | SOAP_XML_INDENT);
    ctx->send_timeout = 3; // 3 sec, stop if server is not accepting msg
    ctx->recv_timeout = 3; // 3 sec, stop if server does not respond in time

    string gui_url;
    gui_url = "http://" + CCfgData::Instance().get_gui_server_ip() + ":" +
              to_string(CCfgData::Instance().get_gui_server_port()) + "/faceview";

    value request(ctx), response(ctx);

    // make the JSON REST POST request and get response
    request["person_id"] = p_face->person_id;
    request["img"] = p_face->image_data;
    request["width"] = p_face->image_width;
    request["height"] = p_face->image_height;

    if (p_face->score * 100 >= CCfgData::Instance().get_pass_similarity() && p_face->p_photo_info != nullptr)
    {
        request["photo_flag"] = 1;
        request["score"] = p_face->score * 100;
        request["photo_id"] = p_face->p_photo_info->photo_person_id;
        request["photo_name"] = p_face->p_photo_info->photo_person_name;
        request["photo_department"] = p_face->p_photo_info->photo_person_department;

        // read photo data
        string photo_path = CCfgData::Instance().get_full_path() + "photos/" + p_face->p_photo_info->photo_path;
        ifstream photo_stream(photo_path, ifstream::in | ifstream::binary);
        if (photo_stream)
        {
            photo_stream.seekg(0, ios::end);
            jpeg_size = photo_stream.tellg();
            photo_stream.seekg(0, ios::beg);
            photo_stream.read((char *) jpeg_data, jpeg_size);

            // base64 encode
            CBase64::Encode(jpeg_data, jpeg_size, base64_data);
            request["photo_img"] = (char *) base64_data;

            Mat jpg_mat = imread(photo_path, CV_LOAD_IMAGE_COLOR);
            //检测是否加载成功
            if (jpg_mat.data)  //or == if(jpg_mat.empty())
            {
                request["photo_width"] = jpg_mat.cols;
                request["photo_height"] = jpg_mat.rows;
            }
        }
    }
    else
    {
        request["photo_flag"] = 0;
    }

    if (json_call(ctx, gui_url.c_str(), request, response))
    {
        stringstream error_message;
        soap_stream_fault(ctx, error_message);
        log_message.clear();
        log_message = "rest failure, errInfo:" + error_message.str();
        WARNING_LOG(log_message);

        rtn = false;
    }
    else
    {
        log_message.clear();
        log_message = "SendImage is successful.";
        INFO_LOG(log_message);
    }

    // clean up
    soap_destroy(ctx);
    soap_end(ctx);
    soap_free(ctx);

    delete[] jpeg_data;
    delete[] base64_data;

    return rtn;
}
