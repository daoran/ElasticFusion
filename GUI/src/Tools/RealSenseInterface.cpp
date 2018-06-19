#include "RealSenseInterface.h"
#include <functional>
#include <thread>

#ifdef WITH_REALSENSE
RealSenseInterface::RealSenseInterface(int inWidth,int inHeight,int inFps)
    : width(inWidth),
    height(inHeight),
    fps(inFps),
    dev(nullptr),
    initSuccessful(true)
{

    auto list = ctx.query_devices();
    if (list.size() == 0) 
        throw std::runtime_error("No device detected. Is it plugged in?");


    rs2::device tmp_dev = list.front();
    dev = &tmp_dev;
    std::cout << "start" << std::endl;
    //dev->enable_stream(rs2::stream::depth,width,height,rs2::format::z16,fps);
    //dev->enable_stream(rs2::stream::color,width,height,rs2::format::rgb8,fps);
    std::cout << dev->get_info(RS2_CAMERA_INFO_NAME) << " " << dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << std::endl; 

    latestDepthIndex.assign(-1);
    latestRgbIndex.assign(-1);

    for(int i = 0; i < numBuffers; i++)
    {
        uint8_t * newImage = (uint8_t *)calloc(width * height * 3,sizeof(uint8_t));
        rgbBuffers[i] = std::pair<uint8_t *,int64_t>(newImage,0);
    }

    for(int i = 0; i < numBuffers; i++)
    {
        uint8_t * newDepth = (uint8_t *)calloc(width * height * 2,sizeof(uint8_t));
        uint8_t * newImage = (uint8_t *)calloc(width * height * 3,sizeof(uint8_t));
        frameBuffers[i] = std::pair<std::pair<uint8_t *,uint8_t *>,int64_t>(std::pair<uint8_t *,uint8_t *>(newDepth,newImage),0);
    }

    //setAutoExposure(true);
    //setAutoWhiteBalance(true);

	rs2::frame_queue queue(numBuffers);
	std::thread t([&, inWidth, inHeight, inFps]() {
        /* rs2 hight level api start*/
        rs2::pipeline pipe;
        rs2::config cfg;
        std::cout<< "width:"<<inWidth <<" Height:"<< inHeight << ", fps:"<<inFps << std::endl;
        cfg.enable_stream(RS2_STREAM_COLOR, inWidth, inHeight, RS2_FORMAT_RGB8,inFps);
        cfg.enable_stream(RS2_STREAM_DEPTH, inWidth, inHeight, RS2_FORMAT_Z16, inFps);
        pipe.start(cfg);
        std::cout<<"NICE!"<<std::endl;

        /* rs2 hight level api end*/
        while (true)
        {
            auto frames = pipe.wait_for_frames();

            lastDepthTime = lastRgbTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

            std::cout << "lastRgbTime = " << lastRgbTime << std::endl;
            std::cout << "lastDepthTime = " << lastDepthTime << std::endl;


            rs2::depth_frame depth_frame = frames.get_depth_frame();
            rs2::video_frame color_frame = frames.get_color_frame();


            int bufferIndex = (latestRgbIndex.getValue() + 1) % numBuffers;
            //RGB

            memcpy(rgbBuffers[bufferIndex].first,color_frame.get_data(),
                    color_frame.get_width() * color_frame.get_height() * 3);

            rgbBuffers[bufferIndex].second = lastRgbTime;
            latestRgbIndex++;


            //Depth
            
            // The multiplication by 2 is here because the depth is actually uint16_t
            memcpy(frameBuffers[bufferIndex].first.first,depth_frame.get_data(),
                    depth_frame.get_width() * depth_frame.get_height() * 2);

            frameBuffers[bufferIndex].second = lastDepthTime;

            int lastImageVal = latestRgbIndex.getValue();

            if(lastImageVal == -1)
            {
                return;
            }

            lastImageVal %= numBuffers;

            memcpy(frameBuffers[bufferIndex].first.second,rgbBuffers[lastImageVal].first,
                    depth_frame.get_width() * depth_frame.get_height() * 3);

            latestDepthIndex++;

        }
    });
    t.detach();
}

RealSenseInterface::~RealSenseInterface()
{
    if(initSuccessful)
    {
        //dev->stop();

        for(int i = 0; i < numBuffers; i++)
        {
            free(rgbBuffers[i].first);
        }

        for(int i = 0; i < numBuffers; i++)
        {
            free(frameBuffers[i].first.first);
            free(frameBuffers[i].first.second);
        }

        //delete rgbCallback;
        //delete depthCallback;
    }
}

void RealSenseInterface::setAutoExposure(bool value)
{
//    dev->set_option(rs2::option::color_enable_auto_exposure,value);
//    rs2_set_option(RS2_OPTION_AUTO_EXPOSURE_MODE,)
}

void RealSenseInterface::setAutoWhiteBalance(bool value)
{
//    dev->set_option(rs2::option::color_enable_auto_white_balance,value);
}

bool RealSenseInterface::getAutoExposure()
{
//    return dev->get_option(rs2::option::color_enable_auto_exposure);
    return false;
}

bool RealSenseInterface::getAutoWhiteBalance()
{
    //return dev->get_option(rs2::option::color_enable_auto_white_balance);
    return false;
}
#else

RealSenseInterface::RealSenseInterface(int inWidth,int inHeight,int inFps)
    : width(inWidth),
    height(inHeight),
    fps(inFps),
    initSuccessful(false)
{
    errorText = "Compiled without Intel RealSense library";
}

RealSenseInterface::~RealSenseInterface()
{
}

void RealSenseInterface::setAutoExposure(bool value)
{
}

void RealSenseInterface::setAutoWhiteBalance(bool value)
{
}

bool RealSenseInterface::getAutoExposure()
{
    return false;
}

bool RealSenseInterface::getAutoWhiteBalance()
{
    return false;
}
#endif
