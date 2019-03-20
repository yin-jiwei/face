#ifndef _HW_FACE_SDK_GPU_HEADER
#define _HW_FACE_SDK_GPU_HEADER

#ifndef APIEXPORT
    #define APIEXPORT
#endif

#ifndef  __HW_FACE_DEF
#define __HW_FACE_DEF

typedef struct HWFaceRect                 // 人脸框，图像左上角为(0,0)
{
	int top;
	int bottom;
	int left;
	int right;
} FaceRect;

typedef struct HWQualityScore {           // 质量判断得分
	int illumination;                     // 人脸光照适宜度分数。取值0~100 . 过亮或过暗得分都比较低
	int sharpness;                        // 人脸清晰程度分数。取值0~100 . 越清晰得分越高
	float roll;                           // 左右歪头角度
	float yaw;                            // 左右侧脸角度
	float pitch;                          // 俯仰角度
	int alignment;	                      // 关键点对齐分数。取值0~100，关键点越完整清晰一般得分越高
    int age;                              // 年龄段信息，取值为 0表示未进行年龄判断，取值1:0-6岁 2:7-19岁 3:20-40岁 4:40-60岁 5:60-80岁
    int sunglasses;                       // 是否戴墨镜。取值0-100，越高表示越可能戴墨镜
    int mask;                             // 是否戴口罩。取值0-100，越高表示越可能戴口罩
} QualityScore;

#endif // __HW_FACE_DEF

// SDK函数返回值
#define HW_FACE_SDK_SUCCESS                     0 // 成功执行
#define HW_FACE_SDK_ERROR_TIME_EXPIRED          1 // SDK过期
#define HW_FACE_SDK_ERROR_INVALID_ARGUMENT      2 // 调用参数有误
#define HW_FACE_SDK_ERROR_MEM_INSUFFICIENT      3 // 内存不足
#define HW_FACE_SDK_ERROR_GPU_MEM_INSUFFICIENT  4 // 显存不足
#define HW_FACE_SDK_ERROR_FILE_NOT_FOUND        5 // 文件未找到
#define HW_FACE_SDK_ERROR_INVALID_FILE          6 // 无效的文件
#define HW_FACE_SDK_ERROR_INVALID_HANDLE        7 // 无效的句柄

#ifdef __cplusplus
extern "C"
{
#endif

    /**************************************************
                    GPU人脸定位 & 特征提取
    /*************************************************/

    /** 获取当前人脸SDK版本。

        @param version_str          存放版本字符串的缓冲区，长度应大于等于20字节。
        @return                     返回HW_FACE_SDK_SUCCESS表明正确执行，其他返回值见相关定义，下同。
    */
    typedef int (*tagHWGetFaceSDKVersion)(
        char *version_str
    );


    /** 获取提取的特征长度。

        @param feature_length       [输出]特征长度，单位为字节。
    */
    typedef int (*tagHWGetFaceFeatureSize)(
        int *feature_length
    );


    /** 初始化人脸SDK句柄。

        初始化句柄时，会分配SDK执行时所需的内存/显存资源，初始化过程比较耗时，应尽量减少初始化调用，资源不足时会返回相应错误码。

        @param handle               [输出]待初始化的句柄，句柄类型为void*。
        @param resource_file        SDK所需的资源文件路径，需要精确到文件名，例如"D:\HWDataXXX.bin"
                                    字符串编码可以使用ASCII、UTF-8、GB2312、GBK
        @param gpu_id               CUDA设备编号，该句柄所有资源将分配在该GPU上，在使用该句柄计算时，所有计算操作也在该GPU上执行。
                                    当系统中只有一块支持CUDA的显卡时，gpu_id应为0。
        
        @note                       句柄不是线程安全的，多个线程不能同时使用一个句柄。
                                    不再使用句柄后应使用HWReleaseFaceSDK进行释放。
    */
    typedef int (*tagHWInitFaceSDK)(
        void **handle, 
        const char *resource_file, 
        int gpu_id
    );


    /** 释放人脸SDK句柄。

        使用HWInitFaceSDK初始化句柄后，在不使用后应调用且仅调用一次该函数，以释放为该句柄分配的相关资源。

        @param handle               待释放的人脸SDK句柄。

        @see                        HWInitFaceSDK
    */
    typedef int (*tagHWReleaseFaceSDK)(
        void* handle
    );


    typedef struct HWFrame{                 // 视频帧以及该帧输出
        // [输入] 该帧的图像信息
        int             frame_id;           // 帧编号(可选)
        void*           image_color;        // 帧的彩色图像，图像格式见FRAME_IMAGE_FORMAT系列宏定义
        void*           image_gray;         // 帧的灰度图像，从图像左上角到右下角逐个像素存储
        // [输出] 定位信息 & 质量判断结果
        int             face_count;         // 该帧定位出的人脸个数
        HWFaceRect*     face_result;        // 存储人脸定位结果的缓冲区，需要外部分配好足够大小空间（每帧最大定位出的人脸数 * sizeof(HWFaceRect)），每帧最大检出人脸数参考HWSetFaceExtractParameter
        HWQualityScore* quality_result;     // 存储人脸质量判断结果的缓冲区，数组前face_count个元素为该帧定位出的各个人脸的质量判断结果，需要外部分配好足够大小（最大定位出人脸数 * sizeof(HWQualityScore)）
        // [输出] 提取的特征信息
        int             feature_count;      // 该帧提取的特征个数。取值范围0~face_count。 若该值等于face_count，说明全部定位出的人脸都提取了特征，否则表示存在部分定位的脸未通过质量判断，所以未提取特征
        char*           have_feature;       // 前face_count个元素表示：该帧定位出的各个脸中哪些提取了特征，若have_feature[i]==1，则说明(feature_result+i*特征长度)开始的内存空间中存放着与人脸(face_result+i)对应的特征，需要外部分配好空间。（最大定位出人脸数 * sizeof(char)）
        unsigned char*  feature_result;     // 存储特征的缓冲区，需要外部分配好足够的内存空间（每帧最大定位出的人脸数 * HWGetFaceFeatureSize返回的特征长度)，每帧最大检出人脸数参考HWSetFaceExtractParameter
    }Frame;

    // 帧彩色图像格式
    #define FRAME_IMAGE_FORMAT_PlanarRGB32f_GPU     0 // RGB彩色图像。依次存储R、G、B三个分量，每个分量上各像素从图像左上到右下排列，类型为float，且图像存储在为显存中。(RRGGBB)
    #define FRAME_IMAGE_FORMAT_PlanarRGB32f         1 // RGB彩色图像。依次存储R、G、B三个分量，每个分量上各像素从图像左上到右下排列，类型为float。(RRGGBB)
    #define FRAME_IMAGE_FORMAT_InterleavedRGB8u     2 // RGB彩色图像。从图像左上角到右下角逐个像素存储，每个像素依次包含R、G、B三个分量，类型为unsigned char。
    #define FRAME_IMAGE_FORMAT_InterleavedBGR8u     3 // RGB彩色图像。从图像左上角到右下角逐个像素存储，每个像素依次包含B、G、R三个分量，类型为unsigned char。

    /** 批量定位多帧图像中的人脸、提取符合质量判断条件的人脸特征。

        该函数使用HWFrame作为各帧输入输出，输入的各个HWFrame结构体需要进行恰当初始化，设置输入图像，该函数执行后，HWFrame的各个输出成员被填充为运行结果。
        每帧检出的最大人脸数和其他功能参数可在HWSetFaceExtractParameter中设置；质量判断条件，可在HWSetMultiFrameQualityParam函数中设置。

        @param handle               使用HWInitFaceSDK初始化后的人脸SDK句柄
        @param frames               待处理的帧数组
        @param frame_count          待处理的帧个数
        @param frame_height         帧高度
        @param frame_width          帧宽度
        @param image_format         帧中彩色图像的储存格式，可取值参考FRAME_IMAGE_FORMAT系列宏定义

        @note                       1、使用该接口时，每帧图像的尺寸必须完全一致；若尺寸不一致，可在保持长宽比的条件下将所有图像缩放至统一合适大小，或每次调用只传入一帧图像。
                                    2、确保HWFrame的各个输出缓冲区有足够空间，所需空间大小由每帧最大定位出的人脸数决定（默认为DEFAULT_MAX_DETECT_FACE_COUNT），详见HWFrame。
        @see                        HWFrame, HWSetFaceExtractParameter, HWSetFaceQualityCheckParameter
    */
    typedef int (*tagHWExtractFaceFeature)(
        void* handle, 
        HWFrame *frames, 
        int frame_count, 
        int frame_height, int frame_width, 
        int image_format
    );

    /** 批量定位多张图像中的人脸、提取符合质量判断条件的人脸特征。
    
        该函数类似HWExtractFaceFeature，存在以下区别：
            1、支持批量处理不同大小的图像，见frame_heights、frame_widths参数。
            2、输入图像不再需要灰度图，只需要传入彩色图像即可。
            3、输入图像格式有所限制，见image_format参数。

        @param handle               使用HWInitFaceSDK初始化后的人脸SDK句柄
        @param frames               待处理的图像HWFrame缓冲区
        @param frame_count          待处理的图像个数
        @param frame_heights        各图像的高度，缓冲区长度至少为frame_count
        @param frame_widths         各图像的宽度，缓冲区长度至少为frame_count
        @param image_format         HWFrame中彩色图像的储存格式，目前只支持FRAME_IMAGE_FORMAT_InterleavedRGB8u和FRAME_IMAGE_FORMAT_InterleavedBGR8u

        @note                       1、使用该接口时，每图像的尺寸最好相对接近；
                                    2、该接口不需要HWFrame的image_gray字段，赋值为NULL即可
                                    3、确保HWFrame的各个输出缓冲区有足够空间，所需空间大小由每帧最大定位出的人脸数决定（默认为DEFAULT_MAX_DETECT_FACE_COUNT），详见HWFrame。
        @see                        HWExtractFaceFeature
    */
    typedef int (*tagHWExtractImagesFaceFeature)(
        void* handle,
        HWFrame *frames,
        int frame_count,
        int* frame_heights, int* frame_widths,
        int image_format
    );

    #define DEFAULT_MAX_DETECT_FACE_COUNT 16
    #define DEFAULT_QUALITY_CHECK_MODE 1
    #define DEFAULT_DETECT_SHRINK_TIMES 2
    #define DEFAULT_FEATURE_EXTRACT_BATCH 8
    #define MAX_FEATURE_EXTRACT_BATCH 64

    /** 设置人脸SDK句柄在定位、提特征时用到的各种参数。

        @param handle               要设置参数的人脸SDK句柄。
        @param max_face_count       每帧最大定位出的人脸数，默认值为DEFAULT_MAX_DETECT_FACE_COUNT
                                    该参数影响创建Frame时，face_result、quality_result、have_feature、feature_result需要分配的空间大小。
                                    若待处理的图像中只包含较少个数的人脸，可用该参数加以限制，可以提高一定定位速度。
        @param quality_check_mode   质量判断模式，默认值为DEFAULT_QUALITY_CHECK_MODE。
                                    取值0为不启用，1为启用，2启用（包含光照模糊度，速度较慢）。
        @param extract_feature_batch_size
                                    提取特征计算时最大批量处理的人脸个数，默认值为DEFAULT_FEATURE_EXTRACT_BATCH。
                                    可设为[1, MAX_FEATURE_EXTRACT_BATCH]间整数，适当提高该值可能提高吞吐量，但会占用更多显存。
        @param img_shrink_times     定位阶段图像缩小的倍数，默认值为DEFAULT_DETECT_SHRINK_TIMES。
                                    提升缩小倍数可提升速度，能也会损失一些较小的人脸；值为2时，最小人脸参考值为40，若设为3，则最小人脸参考值为60，该值不是严格的界限，表示最小尺寸以下的脸相对较难检测到。

        @note                       各个参数会忽略小于0的输入，即如果传小于0的值给某个参数，则本次调用不设置对应参数。
        @see                        HWFrame
    */
    typedef int (*tagHWSetFaceExtractParameter)(
        void* handle,
        int max_face_count,
        int quality_check_mode,
        int extract_feature_batch_size,
        int img_shrink_times
    );

    #define DEFAULT_DETECT_THRESHOLD_LEVEL 3

    /** 设置人脸SDK句柄在定位阶段用到的参数。

        @param handle               要设置参数的人脸SDK句柄。
        @param threshold_level      定位的严格程度，取值如下：
                                    小于等于0 : 不改变设置
                                     1 至 5  : 数值越高越严格，误定位越少，漏检增多；默认值为DEFAULT_DETECT_THRESHOLD_LEVEL
    */
    typedef int (*tagHWSetFaceDetectParameter)(
        void* handle,
        int threshold_level
    );


    #define DEFAULT_MIN_ALIGN_SCORE 90
    #define DEFAULT_MIN_SHARPNESS 40
    #define DEFAULT_MIN_ILLUMINATION 70
    #define DEFAULT_MIN_YAW -45
    #define DEFAULT_MAX_YAW +45
    #define DEFAULT_MIN_PITCH -25
    #define DEFAULT_MAX_PITCH +40
    #define DEFAULT_AGE_FILTER_FLAG 0

    /** 设置质量判断涉及的各种阈值。

        @param handle               要设置质量判断参数的人脸SDK句柄。
        @param min_alignment_score  最小的对齐分数。默认值DEFAULT_MIN_ALIGN_SCORE，质量判断时人脸得分小于该值，将不会提取该脸特征。
        @param min_sharpness        最小清晰度分数。默认值DEFAULT_MIN_SHARPNESS。
        @param min_illumination     最小光照适宜度分数，默认值DEFAULT_MIN_ILLUMINATION。
        @param min_yaw, max_yaw     左右侧脸角度范围，小于min_yaw或大于max_yaw的人脸将不会被提取特征。默认为DEFAULT_MIN_YAW\DEFAULT_MAX_YAW
        @param min_pitch, max_pitch 俯仰角度范围。默认为DEFAULT_MIN_PITCH\DEFAULT_MAX_PITCH
        @param age_filter_flag      年龄判断设置，取0时表示不进行年龄判断；取1时表示进行年龄判断，但不用年龄结果做质量判断；
                                    取大于1的值时，表示进行年龄判断且根据年龄判断进行质量判断，age_filter_flag从最低位向最高位的第2到6位
                                    依次对应由小到大的5个年龄段，某位取1时，表示对应年龄段的人脸被过滤掉，不会提取其特征。
        @param check_sunglass_mask  是否启用戴墨镜、戴口罩检测，设为1则表示启用，默认为0。

        @see                        QualityScore
    */
    typedef int (*tagHWSetFaceQualityCheckParameter)(
        void* handle, 
        int min_alignment_score, 
        int min_sharpness, 
        int min_illumination,
        float min_yaw, float max_yaw, 
        float min_pitch, float max_pitch,
        int age_filter_flag,
        char check_sunglass_mask
    );


    /**************************************************
                        GPU特征比对
    /*************************************************/

    /** 初始化模板库句柄。

        @param handle               待初始化的模板库句柄。
        @param gpu_id               该句柄的相关资源所在GPU编号。
        @param template_library_features        
                                    模板库缓冲区，由feature_count个特征依次排列组成。
        @param feature_count        模板库的特征个数。
    */
    typedef int (*tagHWInitFaceFeatureCompareTemplate)(
        void** handle,
        int gpu_id,
        unsigned char *template_library_features, 
        int feature_count
    );


    /** 初始化比对句柄。

        @param handle               待初始化的比对句柄。
        @param gpu_id               该句柄的相关资源所在GPU编号。
    */
    typedef int (*tagHWInitFaceFeatureCompare)(
        void** handle,
        int gpu_id
    );


    /** 释放模板库句柄或比对句柄的资源。

        @param handle               待释放的句柄，由HWInitFaceFeatureCompareTemplate或HWInitFaceFeatureCompare初始化。
    */
    typedef int (*tagHWReleaseFaceFeatureCompare)(
        void* handle
    );

    #define MAX_COMPARE_FEATURE_COUNT 64 // 调用HWCompareFaceFeature时，待比对特征数量的最大值，即compare_feature_count参数应小于等于该值
    #define MAX_COMPARE_CANDIDATE_COUNT 32 // 调用HWCompareFaceFeature时，获取候选的最大值，即candidate_count参数应小于等于该值

    /** GPU批量特征比对。
        
        将待比对特征和模板库中所有特征进行比对，对于每个待比对特征，返回相似度最高的若干模板（即在特征库的下标和相似度分数）。
        调用时，需传入一个比对句柄和一个特征库句柄，分别由HWInitFaceFeatureCompare和HWInitFaceFeatureCompareTemplate初始化得到。

        @param handle_compare       比对句柄。
        @param handle_template      模板库句柄。
        @param feature_to_compare   待比对特征缓冲区。
        @param compare_feature_count
                                    待比对特征的个数，该值不应超过MAX_COMPARE_FEATURE_COUNT，推荐值16。
        @param candidate_count      对每个待比对特征，要返回的候选个数，该值不应超过MAX_COMPARE_CANDIDATE_COUNT。
        @param result_index         [输出]下标缓冲区，长度至少为compare_feature_count*candidate_count，
                                    其中前candidate_count个元素为：模板库中和首个待比对特征最相似的candidate_count个特征的下标（按相似度从高到底排列），
                                    第candidate_count+1个到candidate_count*2个元素为第二个待比对特征的相应比对结果，以此类推。
                                    若特征库中特征个数小于候选个数，不足部分下标返回-1，对应分数返回0。
        @param result_score         [输出]分数缓冲区，长度至少为compare_feature_count*candidate_count，为result_index对应的相似度分数，分数取值为[0, 1]，越高表示越相似。

        @note                       1、handle_compare句柄包含特征库相关GPU资源，handle_template句柄包含的是和模板库对比时需要的资源
                                       使用时，handle_template可以在多个线程间共享（即和同一个特征库比对），handle_compare则是每个线程使用其各自的（即每个比对线程都需要用HWInitFaceFeatureCompare初始化各自的比对句柄）。
                                    2、调用该接口时，传入的handle_compare和handle_template在初始化时所设置的GPU ID必须一致。
                                    3、在多GPU上多线程比对时，应在每个GPU上初始化一个模板句柄，比对线程按每个GPU分为若干组，每组各线程初始化各自GPU上的比对句柄。
                                    4、candidate_count设为1时，表示只返回比对的首选结果，一般首个候选结果是准确的，后续准确度较低。
    */
    typedef int (*tagHWCompareFaceFeature)(
        void* handle_compare, 
        void* handle_template, 
        unsigned char* feature_to_compare, 
        int compare_feature_count,
        int candidate_count,
        int* result_index, 
        float* result_score
    );


    /** 单独比较两个特征的相似度。

        @param feature_a            待比对特征A。
        @param feature_b            待比对特征B。
        @param score                [输出]相似度结果。

        @note                       1、若要比对的特征对数量较大，应使用HWCompareFaceFeature
    */
    typedef int (*tagHWCompareSingleFaceFeature)(
        unsigned char* feature_a,
        unsigned char* feature_b,
        float* score
    );


#ifdef __cplusplus
}
#endif

#endif // _HW_FACE_SDK_GPU_HEADER