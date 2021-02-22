#ifndef Preview_hpp
#define Preview_hpp

#if defined(__has_warning)
#if __has_warning("-Wreserved-id-macro")
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#endif
#endif

#include <opencv2/core/mat.hpp>  // for basic OpenCV structures (Mat, Scalar)
#include <opencv2/imgcodecs.hpp> // for reading and writing
#include <opencv2/imgproc.hpp>   // for cv::cvtColor()
#include <opencv2/videoio.hpp>

#if defined(__has_warning)
#if __has_warning("-Wdocumentation")
#pragma GCC diagnostic pop
#endif
#endif

#include "Configuration.hpp"

using cv::Mat;

/*----------------------------------------------------------------------------------------------------
    MARK: - Functions
   ----------------------------------------------------------------------------------------------------*/

// Convert an integer representing a number of seconds to a timestamp of the form hh:mm:ss
string secondsToTimeStamp(const double seconds);

// Convert a frame number to a number of seconds (requires knowledge of the fps of the video)
// Rounds down to the nearest integer
double frameNumberToSeconds(const int frameNumber, const int fps);

/*----------------------------------------------------------------------------------------------------
    MARK: - Frame
   ----------------------------------------------------------------------------------------------------*/

// Data and functions relavant to a single frame of a video
class Frame
{
public:
    Frame(const Mat& dataIn, const int frameNumberIn, const double fps)
        : data{ dataIn }, frameNumber{ frameNumberIn }, seconds{ frameNumberToSeconds(frameNumberIn, fps) }
    {}
    
    Mat getData()                     const { return data; }
    int getFrameNumber()              const { return frameNumber; }
    int getFrameNumberHumanReadable() const { return frameNumber + 1; } // OpenCV indexes frames from 0
    string gettimeStampString()       const { return secondsToTimeStamp(seconds); }

private:
    Mat    data;
    int    frameNumber;
    double seconds;
};



/*----------------------------------------------------------------------------------------------------
    MARK: - Video
   ----------------------------------------------------------------------------------------------------*/

// Data and functions relevant to a single video file.
// Essentially a wrapper class for a cv::VideoCapture object
class Video
{
public:
    Video() {};
    
    Video(const string& path) : vc{ path }
    {
        if (!vc.isOpened())
            throw FileException("file either could not be opened or is not an accepted format\n", path);
    }

    int      getFrameNumber()           const { return vc.get(cv::CAP_PROP_POS_FRAMES);  }
    int      getNumberOfFrames()        const { return vc.get(cv::CAP_PROP_FRAME_COUNT); }
    int      getCodec()                 const { return vc.get(cv::CAP_PROP_FOURCC); }
    double   getFPS()                   const { return vc.get(cv::CAP_PROP_FPS);    }
    cv::Size getDimensions()            const { return cv::Size(vc.get(cv::CAP_PROP_FRAME_WIDTH),vc.get(cv::CAP_PROP_FRAME_HEIGHT)); }
    
    void     setFrameNumber(const int num)    { vc.set(cv::CAP_PROP_POS_FRAMES, num); }
    void     writeCurrentFrame(Mat& frameOut) { vc.read(frameOut); }                       // Overwrite `frameOut` with a `Mat` corresponding to the currently selected frame

    // Exports an MJPG to exportDir consisting of frames frameBegin to frameEnd-1. Used for exporting preview videos
    void     exportVideo(const string& exportPath, const int frameBegin, const int frameEnd);

private:
    cv::VideoCapture vc;
};


/*----------------------------------------------------------------------------------------------------
    MARK: - GUIInformation
   ----------------------------------------------------------------------------------------------------*/

class GUIInformation
{
public:
    int getRows()                { return rowsInPreview; }
    int getCols()                { return colsInPreview; }
    
    void setRows(const int rows) { rowsInPreview = rows; previewIsUpToDate = false; }
    void setCols(const int cols) { colsInPreview = cols; previewIsUpToDate = false; }
    
    void previewHasBeenUpdated() { previewIsUpToDate = true; }
    bool isPreviewUpToDate()     { return previewIsUpToDate; }
    
private:
    int rowsInPreview;
    int colsInPreview;
    
    bool previewIsUpToDate = true;
};


/*----------------------------------------------------------------------------------------------------
    MARK: - VideoPreview
   ----------------------------------------------------------------------------------------------------*/

// The main class associated with previewing a single video. `VideoPreview` has three core components:
//      1. `video`:          a `Video` object.            Deals with the core video file which is being previewed
//      2. `frames`:         a vector of `Frame` objects. Deals with the individual frames which are shown in the preview
//      3. `optionsHandler`: a `ConfigOptionsHandler`.    Deals with any options supplied by configuration files
class VideoPreview
{
public:
    VideoPreview(const string& videoPathIn) : videoPath{ videoPathIn } { determineExportPath();}
    
    // Attempts to initialize video with the file at videoPath
    void loadVideo() {
        try {
            video = Video(videoPath);
        } catch (const FileException& exception) {
            std::cerr<< exception.what();
        }
    }
    
    void loadConfig() { optionsHandler = ConfigOptionsHandler{ videoPath }; }

    // Everything that needs to be run in order to update the actual video preview that the user sees
    // To be run on start-up and whenever configuration options are changed
    void updatePreview();

    // Return a `ConfigOptionPtr` to the config option corresponding to `optionID`.
    // If the option isn't currently set, it is set to the default value as given by ConfigOption::recognisedConfigOptions
    // If `optionID` is invalid (doesn't correspond to a recognised option), nullptr is returned.
    // It is up to the caller to check if nullptr has been returned
    ConfigOptionPtr getOption(const string& optionID)
    {
        // Search for optionID in the video previews current set of config options
        // ConfigOptionVector.getOption() return nullptr if the option doesn't exist
        ConfigOptionPtr option = optionsHandler.getOptions().getOption(optionID);
        if (option)
            return option;
        
        // If the optionID wasn't found, search for it inthe recognised configuration options
        auto temp = ConfigOption::recognisedOptionInfo.find(optionID);

        // If optionID was found in recognisedOptionInfo, return a ConfigOptionPtr with the corresponding default value
        if (temp != ConfigOption::recognisedOptionInfo.end())
        {
            ConfigValuePtr  defaultValue = ConfigOption::recognisedOptionInfo.at(optionID).getDefaultValue();
            ConfigOptionPtr newOption    = std::make_shared<ConfigOption>(optionID, defaultValue);
            optionsHandler.setOption(newOption); // set with a smart pointer
            return newOption;
        }
        
        // If the optionID wasn't found in the recognised options, return nullptr
        return nullptr;
    }
    
    void setOption(const string& optionID, const bool val)
    {
        optionsHandler.setOption(optionID, val);
        updatePreview();
    }

    void setOption(const string& optionID, const int val)
    {
        optionsHandler.setOption(optionID, val);
        updatePreview();
    }
    
    void setOption(const string& optionID, const double val)
    {
        optionsHandler.setOption(optionID, val);
        updatePreview();
    }

    void setOption(const string& optionID, const string val)
    {
        optionsHandler.setOption(optionID, val);
        updatePreview();
    }
    
    void setOption(const string& optionID, const char* val)
    {
        setOption(optionID, string(val));
    }

    // Save a set of current configuration options to either 1) a preexisiting configuration file, or 2) an arbitrary new file
    // In the case of 1, the formatting of the file is maintained, but any options that have been changed are overwritten
    // Function overrides allow the file to be passed as either a string, or a ConfigFilePtr
    void saveOptions(ConfigOptionVector options, const ConfigFilePtr& file) { optionsHandler.saveOptions(options, file); }
    void saveOptions(ConfigOptionVector options, const string& filePath);
    
    void saveAllOptions(const ConfigFilePtr& file)                          { optionsHandler.saveAllOptions(file); }
    void saveAllOptions(const string& filePath)                             { saveOptions(optionsHandler.getOptions(), filePath); }
    
    void saveOption (ConfigOptionPtr option, const ConfigFilePtr& file)     { optionsHandler.saveOptions(ConfigOptionVector{option}, file); }
    void saveOption (ConfigOptionPtr option, const string& filePath)        { saveOptions(ConfigOptionVector{option}, filePath); }

    void printConfig() const
    {
        cout << "Current configuration options:\n";
        optionsHandler.print();
    }
    
    string getVideoNameString()        { return videoPath; }
    string getVideoNumOfFramesString() { return std::to_string(video.getNumberOfFrames()); }
    
    wstring getVideoDimensionsString() // use wstring to support the unicode times symbol
    {
        cv::Size dims { video.getDimensions() };
        return std::to_wstring(dims.width) + L"\u00d7" + std::to_wstring(dims.height);
    }
    
    string getVideoFPSString()
    {
        std::stringstream ss;
        ss << video.getFPS() << " fps";
        return ss.str();
    }
    
    string getVideoCodecString()
    {   // Refer to https://docs.opencv.org/2.4/doc/tutorials/highgui/video-write/video-write.html
        int ex = video.getCodec();
        char EXT[] = {static_cast<char>(ex & 0XFF) , static_cast<char>((ex & 0XFF00) >> 8),static_cast<char>((ex & 0XFF0000) >> 16),static_cast<char>((ex & 0XFF000000) >> 24), 0};
        return EXT;
    }
    
    string getVideoLengthString()
    {
        int    seconds = frameNumberToSeconds(video.getNumberOfFrames(), video.getFPS());
        return secondsToTimeStamp(seconds);
    }
    
    int getVideoNumOfFrames() { return video.getNumberOfFrames(); }
    
    double getVideoAspectRatio()
    {
        cv::Size dims = video.getDimensions();
        return dims.width / static_cast<double>(dims.height);
        
    }
    
    const static OptionInformation getOptionInformation(const string& optionID) { return ConfigOption::recognisedOptionInfo.at(optionID); } // TODO: make this safe
    
    vector<string> getConfigFilePaths()
    {
        vector<ConfigFilePtr> files = optionsHandler.getConfigFiles();
        vector<string> filePaths;
        filePaths.reserve(files.size());
        for (ConfigFilePtr file : files)
            filePaths.push_back(file->getFilePath());
        return filePaths;
    }
    
    vector<Frame> getFrames()                      { return frames; }
    size_t        getNumOfFrames()                 { return frames.size(); }
    
    void          setRowsInPreview(const int rows) { guiInfo.setRows(rows); }
    void          setColsInPreview(const int cols) { guiInfo.setCols(cols); }
    int           getRowsInPreview()               { return guiInfo.getRows(); }
    int           getColsInPreview()               { return guiInfo.getCols(); }
    
    ~VideoPreview()
    {
        fs::remove_all(exportDir.erase(exportDir.length())); // Delete the temporary directory assigned to this file (remove trailing slash from exportDir)
        if (fs::exists("media/.videopreview") && fs::is_empty("media/.videopreview")) // Delete .videopreview directory if it is empty (i.e. no other file is being previewed)
            fs::remove("media/.videopreview");
    }
    
private:
    // Parse `videopath` in order to determine the directory to which temporary files should be stored
    // This is saved to `exportDir`, and also returned from the function
    // Modified from https://stackoverflow.com/a/8520815
    string& determineExportPath();

    // Read in appropriate configuration options and write over the `frames` vector
    void makeFrames();

    // Exports a "preview video" for each frame in the `frames` vector
    void exportPreviewVideos();

    // Determine if a given configuration option has been changed since the last time the preview was updated
    // Achieved by comparing the relevant `ConfigOptionPtr`s in `currentPreviewConfigOptions` and `optionsHandler`
    bool configOptionHasBeenChanged(const string& optionID)
    {
        // When the program runs for the first time the configuration options have always, by definition, been "changed"
        static bool runningForFirstTime = true;
        if (runningForFirstTime)
        {
            runningForFirstTime = false;
            return true;
        }
        
        ConfigOptionPtr optionInternal{ optionsHandler.getOptions().getOption(optionID) };
        ConfigOptionPtr optionPreview { currentPreviewConfigOptions.getOption(optionID) };
        
        // If the option isn't defined in one of the vectors, it can only be unchanged if they are both equal to each other (i.e. nullptr)
        if (!optionInternal || !optionPreview)
            return optionInternal != optionPreview;

        // Knowing that neither option is a nullptr, we can safely compare the actual values stored in each option
        return optionInternal->getValueAsString() != optionPreview->getValueAsString();
    }

private:
    string               videoPath;                   // Path to the video file
    string               videoDir;                    // Path to the directory containing the video file
    string               exportDir;                   // Path to the directory for exporting temporary files to
    Video                video;
    ConfigOptionsHandler optionsHandler;
    ConfigOptionVector   currentPreviewConfigOptions; // The configuration options corresponding to the current preview (even if internal options have been changed)
    vector<Frame>        frames;                      // Vector of each Frame in the preview
    GUIInformation       guiInfo;
};

#endif /* Preview_hpp */