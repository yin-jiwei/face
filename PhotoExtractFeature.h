//
// Created by yjw on 10/24/18.
//

#ifndef FACE_PHOTOEXTRACTFEATURE_H
#define FACE_PHOTOEXTRACTFEATURE_H

#include <mutex>
#include "Face_1_N.h"

struct PhotoInfo;

class PhotoExtractFeature
{
public:
    PhotoExtractFeature();

    virtual ~PhotoExtractFeature();

    void ExtractFeature();

    bool ExtractFeature(std::string &photo_id,
                        std::string &compare_id,
                        std::string &photo_name,
                        std::string &photo_department,
                        const char *image_data64);

    bool RetrieveFace(const char *image_data64, float &score, PhotoInfo **pp_photo_info, char *face_data);

    bool RetrieveFace(const char *image_data64, int &score, char *face_data);

    bool RetrieveFace(PhotoInfo *p_photo_info);

    void Load_Features_N();

private:
    Feature_Extract feature_extract_;
    Face_1_N face_1_n_;

    unsigned char *jpeg_data_ = nullptr;
    int jpeg_size_ = 0;

    unsigned char *jpeg_feature_ = nullptr;
    int feature_size_ = 0;

    std::mutex safe_mutex_;
};


#endif //FACE_PHOTOEXTRACTFEATURE_H
