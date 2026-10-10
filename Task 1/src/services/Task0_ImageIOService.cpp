#include "services/Task0_ImageIOService.h"

#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================================
// Task 0 - Image Input
// Read RGB (color) and grayscale images and write results back to disk.
// (Using OpenCV for I/O is explicitly allowed.)
// ============================================================================

namespace {

// Loads the raw file bytes - used as a fallback for paths that contain
// non-ASCII characters (Arabic, accents, ...). cv::imread / cv::imwrite only
// understand ANSI paths on Windows, while the Qt UI hands us UTF-8.
bool readFileBytes(const std::string& filePath, std::vector<uchar>& buffer)
{
    // u8path interprets the narrow string as UTF-8 and turns it into the wide
    // path Windows needs, so std::ifstream can open such files as well.
    std::ifstream file(std::filesystem::u8path(filePath), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !buffer.empty();
}

// ".PNG" / "png" / "" -> ".png"
std::string normalizedExtension(const std::string& filePath)
{
    const std::string::size_type dot = filePath.find_last_of('.');
    const std::string::size_type separator = filePath.find_last_of("/\\");

    if (dot == std::string::npos || (separator != std::string::npos && dot < separator)) {
        return {}; // no extension at all
    }

    std::string extension = filePath.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension;
}

} // namespace

// ---------------------------------------------------------------------------
// Reads an image from disk.
//   * IMREAD_UNCHANGED keeps a colour image as 3 channel BGR AND a grayscale
//     image as 1 channel - so both input kinds stay exactly as they are on
//     disk (16 bit and alpha channels survive as well).
//   * Returns an empty Mat when the file does not exist / is not an image.
// ---------------------------------------------------------------------------
cv::Mat Task0_ImageIOService::read(const std::string& filePath)
{
    if (filePath.empty()) {
        return {};
    }

    // 1) Fast path - plain ASCII paths.
    cv::Mat image = cv::imread(filePath, cv::IMREAD_UNCHANGED);
    if (!image.empty()) {
        return image;
    }

    // 2) Fallback for non-ASCII paths: decode the bytes ourselves.
    std::vector<uchar> buffer;
    if (!readFileBytes(filePath, buffer)) {
        return {};
    }

    return cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
}

// ---------------------------------------------------------------------------
// Writes the given image to disk. The format comes from the file extension
// (png / jpg / bmp ...). Returns false when the image is empty or the file
// cannot be created.
// ---------------------------------------------------------------------------
bool Task0_ImageIOService::write(const std::string& filePath, const cv::Mat& image)
{
    if (filePath.empty() || image.empty()) {
        return false;
    }

    // 1) Fast path - plain ASCII paths.
    if (cv::imwrite(filePath, image)) {
        return true;
    }

    // 2) Fallback for non-ASCII paths: encode into memory and write the
    //    bytes with a wide-character aware stream.
    const std::string extension = normalizedExtension(filePath);
    if (extension.empty()) {
        return false; // without an extension OpenCV cannot pick a codec
    }

    std::vector<uchar> buffer;
    if (!cv::imencode(extension, image, buffer)) {
        return false;
    }

    std::ofstream file(std::filesystem::u8path(filePath), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
    return file.good();
}
