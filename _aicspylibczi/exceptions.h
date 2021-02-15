#ifndef _PYLIBCZI_EXCEPTIONS_H
#define _PYLIBCZI_EXCEPTIONS_H

#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "inc_libCZI.h"
#include "pylibczi_ostream.h"

namespace pylibczi {

class FilePtrException : public std::runtime_error
{
public:
  explicit FilePtrException(const std::string& message_)
    : std::runtime_error("File Pointer Exception: " + message_)
  {}
};

class PixelTypeException : public std::runtime_error
{
  static const std::map<libCZI::PixelType, const std::string> s_byName;

public:
  PixelTypeException(libCZI::PixelType pixel_type_, const std::string& message_)
    : std::runtime_error("")
  {
    auto tname = s_byName.find(pixel_type_);
    std::string name((tname == s_byName.end()) ? "Unknown type" : tname->second);
    static_cast<std::runtime_error&>(*this) = std::runtime_error("PixelType( " + name + " ): " + message_);
  }
};

class RegionSelectionException : public std::runtime_error
{
public:
  RegionSelectionException(const libCZI::IntRect& requested_box_,
                           const libCZI::IntRect& image_box_,
                           const std::string& message_)
    : std::runtime_error("")
  {
    std::stringstream front;  // x ∉ Y means x is not an element of Y, or x is not in Y
    front << "Requirement violated requested region is not a subset of the "
             "defined image! \n\t "
          << requested_box_ << " ⊄ " << image_box_ << "\n\t" << message_ << std::endl;
    static_cast<std::runtime_error&>(*this) = std::runtime_error(front.str());
  }
};

class ImageAccessUnderspecifiedException : public std::runtime_error
{
public:
  ImageAccessUnderspecifiedException(size_t given_, size_t required_, const std::string& message_)
    : std::runtime_error("Dimensions underspecified, given " + std::to_string(given_) + " dimensions but " +
                         std::to_string(required_) + " needed! \n\t" + message_)
  {}
};

class ImageIteratorException : public std::runtime_error
{
public:
  explicit ImageIteratorException(const std::string& message_)
    : std::runtime_error("ImageIteratorException: " + message_)
  {}
};

class ImageSplitChannelException : public std::runtime_error
{
public:
  ImageSplitChannelException(const std::string& message_, int channel_)
    : std::runtime_error("ImageSplitChannelExcetion: " + message_ +
                         " Channel should be zero or unset but has a value of " + std::to_string(channel_) +
                         " not sure how to procede in assigning channels.")
  {}
};

class ImageCopyAllocFailed
  : public std::runtime_error
  , public std::bad_alloc
{
public:
  ImageCopyAllocFailed(const std::string& message_, unsigned long alloc_size_)
    : std::runtime_error("")
  {
    std::stringstream tmp;
    auto gbSize = static_cast<double>(alloc_size_);
    gbSize /= 1073741824.0; // 1024 * 1024 * 1024
    tmp << "ImageCopyAllocFailed [" << std::setprecision(1) << gbSize << " GB requested]: " << message_ << std::endl;
    static_cast<std::runtime_error&>(*this) = std::runtime_error(tmp.str());
  }

  // both parent classes define what so it's necessary to override in order to call the appropriate one.
  const char* what() const noexcept override { return std::runtime_error::what(); }
};

class CDimCoordinatesOverspecifiedException : public std::runtime_error
{
public:
  explicit CDimCoordinatesOverspecifiedException(const std::string& message_)
    : std::runtime_error("The coordinates are overspecified = you have specified a Dimension "
                         "or Dimension value that is not valid. " +
                         message_)
  {}
};

class CDimCoordinatesUnderspecifiedException : public std::runtime_error // std::exception
{
public:
  explicit CDimCoordinatesUnderspecifiedException(const std::string& message_)
    : std::runtime_error("The coordinates are underspecified = you have not specified a "
                         "Dimension that is required. " +
                         message_)
  {}
};

class CdimSelectionZeroImagesException : public std::runtime_error
{
public:
  CdimSelectionZeroImagesException(libCZI::CDimCoordinate& requested_plane_coordinate_,
                                   libCZI::CDimBounds& plane_coordinate_bounds_,
                                   const std::string& message_)
    : std::runtime_error("")
  {
    std::stringstream tmp;
    tmp << "Specified Dims resulted in NO image frames: " << requested_plane_coordinate_ << " ∉ "
        << plane_coordinate_bounds_ << " " << message_ << std::endl;
    static_cast<std::runtime_error&>(*this) = std::runtime_error(tmp.str());
  }
};

class StrideAssumptionExcetpion : public std::runtime_error
{
public:
  explicit StrideAssumptionExcetpion(const std::string& message_)
    : std::runtime_error("Image Stride % Width != 0. Please create an issue at "
                         "https://github.com/AllenCellModeling/aicspylibczi "
                         "your file represents a case I didn't realized existed. "
                         "If you can share the file that would be exceptionally helpful. Thank you!"
                         + message_)
  {}
};

}

#endif //_PYLIBCZI_EXCEPTIONS_H
